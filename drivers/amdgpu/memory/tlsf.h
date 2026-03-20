#pragma once
#include <cstdint>
#include <cassert>
#include <cstring>
#include <iostream>
#include "alloc.h"

// Two-Level Segregated Fit allocator.
//
// Architecture (based on VMA's VmaBlockMetadata_TLSF):
//
//   Null block — a permanent sentinel that always occupies the tail of the
//   address space.  It is never placed in the free lists; instead it is
//   checked explicitly as a last resort.  Its size shrinks as allocations
//   consume it and grows as adjacent freed blocks are merged back into it.
//   Having a null block means nextPhys is never nullptr for any real (non-
//   null) block, which simplifies every split/merge path.
//
//   Free/taken encoding — a block's free state is stored inside prevFree_:
//     prevFree_ == this    → block is live ("taken")
//     prevFree_ == nullptr → block is free AND is the head of its bucket list
//     prevFree_ == other   → block is free AND has a predecessor in the bucket
//   nextFree_ and userData_ share a union: a taken block carries caller
//   user-data; a free block carries its next-in-list pointer.  This keeps
//   sizeof(Block) minimal with no separate bool field.
//
//   Alignment / front-padding (from VMA's Alloc()):
//   When a chosen block starts before the required aligned offset, the gap
//   bytes are handled by preferentially growing the previous free block into
//   the gap (cheap, no new block needed).  Only if the previous block is not
//   free is a new taken "gap block" created.  This avoids filling the free
//   lists with tiny padding blocks.

class TlsfAllocator {
public:
    // -------------------------------------------------------------------------
    // Public types
    // -------------------------------------------------------------------------

    // Allocation is the sole currency: keep it alive for the lifetime of the
    // allocation and pass it to free().  _block is opaque — do not touch.
    struct Allocation {
        uint64_t offset   = 0;
        uint64_t size     = 0;
        void*    userData = nullptr;
        void*    _block   = nullptr;   // opaque Block*, do not use directly
    };

    struct Stats {
        uint64_t totalSize        = 0;
        uint64_t freeBytes        = 0;   // free blocks, excludes null block
        uint64_t usedBytes        = 0;
        uint64_t nullBlockSize    = 0;   // uncommitted tail
        uint64_t largestFreeBlock = 0;   // includes null block
        uint32_t freeBlockCount   = 0;   // excludes null block
        uint32_t usedBlockCount   = 0;
    };

    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    explicit TlsfAllocator(uint64_t size)
        : m_totalSize(size)
    {
        assert(size > 0 && "heap size must be > 0");

        memset(&m_flBitmap, 0, sizeof(m_flBitmap));
        memset(m_slBitmap,  0, sizeof(m_slBitmap));
        memset(m_freeLists, 0, sizeof(m_freeLists));

        // The null block starts as the entire heap.
        m_nullBlock          = m_blockPool.alloc();
        *m_nullBlock         = Block{};
        m_nullBlock->offset  = 0;
        m_nullBlock->size    = size;
        m_nullBlock->markFree();
        m_nullBlock->nextFree() = nullptr;
        m_nullBlock->prevFree() = nullptr;

#ifdef DEBUG
        validate();
#endif
    }

    // -------------------------------------------------------------------------
    // allocate
    // -------------------------------------------------------------------------

    bool allocate(uint64_t size, uint64_t alignment, Allocation& out)
    {
        assert(size >= 1         && "size must be > 0");
        assert(isPow2(alignment) && "alignment must be a power of two");
        if (alignment < 1) alignment = 1;

        uint64_t sizeNeeded = size + alignment - 1;

        uint32_t listIdx = 0;
        Block*   block   = findFreeBlock(sizeNeeded, listIdx);

        if (!block) {
            if (m_nullBlock->size >= sizeNeeded)
                block = m_nullBlock;
            else
                return false;
        }

        commitAlloc(block, size, alignment, out);
        return true;
    }

    // -------------------------------------------------------------------------
    // free
    // -------------------------------------------------------------------------

    void free(const Allocation& alloc)
    {
        assert(alloc._block && "null allocation");
        Block* block = static_cast<Block*>(alloc._block);
        assert(!block->isFree() && "double free");

        Block* next = block->nextPhys;   // always non-null (null block is last)
        Block* prev = block->prevPhys;

        // Merge with previous neighbour if free.
        if (prev && prev->isFree()) {
            removeFreeBlock(prev);
            mergeInto(block, prev);   // block grows backward, prev is freed
        }

        // Merge with next neighbour.
        if (next == m_nullBlock) {
            mergeInto(m_nullBlock, block);   // null block grows backward
        } else if (next->isFree()) {
            removeFreeBlock(next);
            mergeInto(next, block);          // next grows backward, block is freed
            insertFreeBlock(next);
        } else {
            block->markTaken();              // required before insertFreeBlock
            insertFreeBlock(block);
        }

#ifdef DEBUG
        validate();
#endif
    }

    // -------------------------------------------------------------------------
    // stats
    // -------------------------------------------------------------------------

    Stats stats() const
    {
        Stats s{};
        s.totalSize     = m_totalSize;
        s.nullBlockSize = m_nullBlock->size;
        s.largestFreeBlock = m_nullBlock->size;

        for (const Block* b = m_nullBlock->prevPhys; b; b = b->prevPhys) {
            if (b->isFree()) {
                ++s.freeBlockCount;
                s.freeBytes += b->size;
                if (b->size > s.largestFreeBlock)
                    s.largestFreeBlock = b->size;
            } else {
                ++s.usedBlockCount;
                s.usedBytes += b->size;
            }
        }
        return s;
    }

    // -------------------------------------------------------------------------
    // validate / debugVisual
    // -------------------------------------------------------------------------

    void validate() const
    {
        validateFreeLists();
        validateBitmaps();
        validatePhysicalList();
    }

    void debugVisual() const
    {
        static constexpr int W = 80;
        char line[W];
        memset(line, '.', W);

        for (const Block* b = m_nullBlock; b; b = b->prevPhys) {
            int s = (int)((b->offset * W) / m_totalSize);
            int e = (int)(((b->offset + b->size) * W) / m_totalSize);
            char c = (b == m_nullBlock) ? '~' : (b->isFree() ? '_' : '#');
            for (int i = s; i < e && i < W; ++i) line[i] = c;
        }
        std::cout << std::string(line, W) << "\n";
    }

private:
    // -------------------------------------------------------------------------
    // Constants
    // -------------------------------------------------------------------------

    static constexpr int SL_LOG2  = 5;
    static constexpr int SL_COUNT = 1 << SL_LOG2;   // 32
    static constexpr int FL_COUNT = 32;

    // -------------------------------------------------------------------------
    // Block
    // -------------------------------------------------------------------------

    struct Block {
        uint64_t offset = 0;
        uint64_t size   = 0;

        Block* prevPhys = nullptr;
        Block* nextPhys = nullptr;

        // Free/taken encoded in prevFree_ (see class comment).
        Block* prevFree_ = nullptr;
        union {
            Block* nextFree_;
            void*  userData_;
        };

        void markFree()  { prevFree_ = nullptr; }
        void markTaken() { prevFree_ = this; userData_ = nullptr; }
        bool isFree() const { return prevFree_ != this; }

        Block*& prevFree() { return prevFree_; }
        Block*& nextFree() { assert(isFree());  return nextFree_; }
        void*&  userData() { assert(!isFree()); return userData_; }
    };

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    uint64_t             m_totalSize;
    Block*               m_nullBlock = nullptr;

    uint32_t             m_flBitmap = 0;
    uint32_t             m_slBitmap[FL_COUNT]{};
    Block*               m_freeLists[FL_COUNT][SL_COUNT]{};

    SlabAllocator<Block> m_blockPool;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static bool isPow2(uint64_t x) { return x && !(x & (x - 1)); }

    static uint64_t alignUp(uint64_t x, uint64_t a)
    {
        return (x + a - 1) & ~(a - 1);
    }

    static int bsf(uint32_t x) { assert(x); return __builtin_ctz(x);    }
    static int msb(uint64_t x) { assert(x); return 63 - __builtin_clzll(x); }

    // Map size to a (fl, sl) index.  Valid for any size >= 1.
    static void sizeToIndex(uint64_t size, int& fl, int& sl)
    {
        assert(size >= 1);
        if (size <= (uint64_t)SL_COUNT) {
            fl = 0;
            sl = (int)size - 1;
        } else {
            fl = msb(size);
            if (fl >= FL_COUNT) fl = FL_COUNT - 1;
            sl = (int)((size >> (fl - SL_LOG2)) & (SL_COUNT - 1));
        }
    }

    // -------------------------------------------------------------------------
    // Free-list operations
    // -------------------------------------------------------------------------

    // Insert block into the appropriate free list.
    // Precondition: block->markTaken() has been called (isFree() == false).
    // The function transitions it to free as part of the operation.
    void insertFreeBlock(Block* b)
    {
        assert(!b->isFree() && "call markTaken() before insertFreeBlock()");
        assert(b != m_nullBlock);

        int fl, sl;
        sizeToIndex(b->size, fl, sl);

        b->prevFree() = nullptr;
        b->nextFree() = m_freeLists[fl][sl];
        m_freeLists[fl][sl] = b;

        if (b->nextFree())
            b->nextFree()->prevFree() = b;
        else {
            m_slBitmap[fl] |= (1u << sl);
            m_flBitmap     |= (1u << fl);
        }
    }

    // Remove block from its free list and mark it taken.
    void removeFreeBlock(Block* b)
    {
        assert(b->isFree());
        assert(b != m_nullBlock);

        int fl, sl;
        sizeToIndex(b->size, fl, sl);

        if (b->nextFree())
            b->nextFree()->prevFree() = b->prevFree();

        if (b->prevFree())
            b->prevFree()->nextFree() = b->nextFree();
        else {
            // b was the list head
            m_freeLists[fl][sl] = b->nextFree();
            if (!m_freeLists[fl][sl]) {
                m_slBitmap[fl] &= ~(1u << sl);
                if (!m_slBitmap[fl])
                    m_flBitmap &= ~(1u << fl);
            }
        }

        b->markTaken();
    }

    // Find the smallest free block whose size >= 'size'.
    // Never returns the null block.
    Block* findFreeBlock(uint64_t size, uint32_t& outListIdx) const
    {
        int fl, sl;
        sizeToIndex(size, fl, sl);

        // Look for a suitable SL bin within this FL class.
        uint32_t slMap = m_slBitmap[fl] & (~0u << sl);
        if (slMap) {
            sl = bsf(slMap);
            outListIdx = fl * SL_COUNT + sl;
            return m_freeLists[fl][sl];
        }

        // No match in this FL class — search higher FL classes.
        uint32_t flMap = m_flBitmap & (~0u << (fl + 1));
        if (!flMap) return nullptr;

        fl = bsf(flMap);
        sl = bsf(m_slBitmap[fl]);
        outListIdx = fl * SL_COUNT + sl;
        return m_freeLists[fl][sl];
    }

    // -------------------------------------------------------------------------
    // mergeInto(dst, src)
    //
    // Absorbs 'src' into 'dst'.  src must be the physical predecessor of dst
    // (src->nextPhys == dst).  src must already be removed from the free list
    // (i.e. markTaken()).  dst grows backward to cover src's range.
    // src is returned to the block pool.
    // -------------------------------------------------------------------------

    void mergeInto(Block* dst, Block* src)
    {
        assert(src->nextPhys == dst);
        assert(!src->isFree() && "remove src from free list before merging");

        dst->offset   = src->offset;
        dst->size    += src->size;
        dst->prevPhys = src->prevPhys;
        if (dst->prevPhys)
            dst->prevPhys->nextPhys = dst;

        m_blockPool.free(src);
    }

    // -------------------------------------------------------------------------
    // commitAlloc
    //
    // Carve an allocation of 'size' bytes at an aligned offset out of 'block'.
    // 'block' may be a regular free block (already removed from free lists, so
    // marked taken) or the null block (never in free lists, stays marked free
    // until we explicitly markTaken it below).
    // -------------------------------------------------------------------------

    void commitAlloc(Block* block, uint64_t size, uint64_t alignment,
                     Allocation& out)
    {
        const bool isNull = (block == m_nullBlock);

        if (!isNull)
            removeFreeBlock(block);   // transitions to taken
        // null block is not in free lists; we markTaken it at the end

        // --- front padding ---------------------------------------------------

        uint64_t alignedOffset = alignUp(block->offset, alignment);
        uint64_t padding       = alignedOffset - block->offset;

        if (padding > 0) {
            Block* prev = block->prevPhys;

            if (prev && prev->isFree()) {
                // Grow the existing previous free block into the gap.
                // If the size increase crosses a bin boundary, re-bucket it.
                int oldFl, oldSl;
                sizeToIndex(prev->size, oldFl, oldSl);
                prev->size += padding;
                int newFl, newSl;
                sizeToIndex(prev->size, newFl, newSl);

                if (oldFl != newFl || oldSl != newSl) {
                    removeFreeBlock(prev);   // marks taken
                    insertFreeBlock(prev);   // marks free again in new bin
                }
                // else: same bin, in-place size update is sufficient

            } else {
                // No usable previous free block — insert a taken gap block.
                Block* gap    = m_blockPool.alloc();
                *gap          = Block{};
                gap->offset   = block->offset;
                gap->size     = padding;
                gap->prevPhys = block->prevPhys;
                gap->nextPhys = block;
                gap->markTaken();

                if (gap->prevPhys)
                    gap->prevPhys->nextPhys = gap;

                block->prevPhys = gap;
            }

            block->offset = alignedOffset;
            block->size  -= padding;
        }

        // --- tail split ------------------------------------------------------

        if (block->size > size) {
            Block* tail    = m_blockPool.alloc();
            *tail          = Block{};
            tail->offset   = block->offset + size;
            tail->size     = block->size - size;
            tail->prevPhys = block;
            tail->nextPhys = block->nextPhys;

            if (tail->nextPhys)
                tail->nextPhys->prevPhys = tail;

            block->nextPhys = tail;
            block->size     = size;

            if (isNull) {
                // Tail becomes the new null block.
                tail->markFree();
                tail->nextFree() = nullptr;
                tail->prevFree() = nullptr;
                m_nullBlock = tail;
            } else {
                // Regular tail — free it.
                tail->markTaken();
                insertFreeBlock(tail);
            }

        } else if (isNull) {
            // Consumed the entire null block — create a zero-size null sentinel.
            Block* newNull    = m_blockPool.alloc();
            *newNull          = Block{};
            newNull->offset   = block->offset + size;
            newNull->size     = 0;
            newNull->prevPhys = block;
            newNull->nextPhys = nullptr;
            newNull->markFree();
            newNull->nextFree() = nullptr;
            newNull->prevFree() = nullptr;

            block->nextPhys = newNull;
            m_nullBlock     = newNull;
        }

        // Finalise the allocated block.
        block->markTaken();
        block->userData() = nullptr;

        out.offset   = block->offset;
        out.size     = block->size;
        out.userData = nullptr;
        out._block   = block;

#ifdef DEBUG
        validate();
#endif
    }

    // -------------------------------------------------------------------------
    // Validation helpers
    // -------------------------------------------------------------------------

    void validatePhysicalList() const
    {
        assert(m_nullBlock);
        assert(m_nullBlock->nextPhys == nullptr && "null block must be last");

        uint64_t total    = m_nullBlock->size;
        uint64_t expected = m_nullBlock->offset;

        for (const Block* b = m_nullBlock->prevPhys; b; b = b->prevPhys) {
            assert(b->size > 0             && "zero-size block in physical list");
            assert(b->offset + b->size == expected && "non-contiguous blocks");
            assert(b->nextPhys->prevPhys == b      && "broken prevPhys link");
            expected = b->offset;
            total   += b->size;
        }

        assert(total    == m_totalSize && "physical list size mismatch");
        assert(expected == 0           && "physical list does not start at 0");
    }

    void validateFreeLists() const
    {
        for (int fl = 0; fl < FL_COUNT; ++fl) {
            for (int sl = 0; sl < SL_COUNT; ++sl) {
                const Block* b    = m_freeLists[fl][sl];
                const Block* prev = nullptr;

                while (b) {
                    assert(b != m_nullBlock       && "null block must not be in free lists");
                    assert(b->isFree()             && "non-free block in free list");
                    assert(b->prevFree_ == prev    && "broken prevFree link");

                    int fl2, sl2;
                    sizeToIndex(b->size, fl2, sl2);

                    if (fl != fl2 || sl != sl2) {
                        std::cout << "Free list mismatch!\n"
                                  << "  Expected bin: (" << fl  << ", " << sl  << ")\n"
                                  << "  Actual bin:   (" << fl2 << ", " << sl2 << ")\n"
                                  << "  Block size:   " << b->size   << "\n"
                                  << "  Offset:       " << b->offset << "\n";
                        debugVisual();
                        assert(false);
                    }

                    prev = b;
                    b    = b->nextFree_;
                }
            }
        }
    }

    void validateBitmaps() const
    {
        for (int fl = 0; fl < FL_COUNT; ++fl) {
            bool flExp = (m_slBitmap[fl] != 0);
            bool flAct = (m_flBitmap & (1u << fl)) != 0;
            assert(flExp == flAct && "flBitmap inconsistent with slBitmap");

            for (int sl = 0; sl < SL_COUNT; ++sl) {
                bool slExp = (m_freeLists[fl][sl] != nullptr);
                bool slAct = (m_slBitmap[fl] & (1u << sl)) != 0;
                assert(slExp == slAct && "slBitmap inconsistent with free list");
            }
        }
    }
};
