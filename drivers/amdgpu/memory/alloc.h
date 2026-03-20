#pragma once
#include <cstdlib>
#include <vector>
#include <cassert>
#include <cstdint>
#include <new>

// random access allocator for predetermined blocks.
//
// tip: elements must be at least 4-bytes sized, else they will be padded to 4-byte boundary.
//
// @todo: consider adding a list of available blocks; when frees occur on older blocks we never reuse that memory.
template<typename T, uint32_t BlockSize = 65536>
class SlabAllocator {
    static_assert(std::is_trivially_destructible<T>::value,
        "SlabAllocator only supports trivially destructible types to prevent memory leaks on destruction.");
    static_assert((BlockSize & (BlockSize - 1)) == 0, "BlockSize must be a power of 2");
public:
    explicit SlabAllocator() : m_head(nullptr), m_avail(nullptr) {
        create_next_block();
    }

    ~SlabAllocator() {
        Block *curr = m_head;
        while(curr) {
            Block* next = curr->next;
            ::free(curr);
            curr = next;
        }
    }

    template<typename... Args>
    T* alloc(Args&&... args) {
        if (!m_avail) {
            create_next_block();
        }

        Block *b = m_avail;
        auto items = get_items(b);
        auto item = &items[b->first_free];
        b->first_free = item->next;

        if (b->first_free == UINT32_MAX) {
            m_avail = b->next_avail;
            b->next_avail = nullptr;
        }

        T* obj = reinterpret_cast<T*>(item->storage);
        new (obj) T{std::forward<Args>(args)...};
        return obj;
    }

    void free(T* ptr) {
        if (!ptr) return;

        Block *b = get_block(ptr);
        Item *item = reinterpret_cast<Item*>(ptr);
        Item *items_base = get_items(b);

        if (b->first_free == UINT32_MAX) {
            b->next_avail = m_avail;
            m_avail = b;
        }

        uint32_t index = static_cast<uint32_t>(item - items_base);
        item->next = b->first_free;
        b->first_free = index;
    }

private:
    union Item {
        uint32_t next;
        alignas(T) unsigned char storage[sizeof(T)];
    };

    struct Block {
        Block *next;
        Block *next_avail;
        uint32_t first_free;
    };

    static Block *get_block(T *p) {
        uintptr_t mask = ~(uintptr_t)(BlockSize - 1);
        Block* b = reinterpret_cast<Block*>((uintptr_t)p & mask);
        return b;
    }

    static Item *get_items(Block *b) {
        uintptr_t header_end = reinterpret_cast<uintptr_t>(b) + sizeof(Block);
        uintptr_t aligned_start = (header_end + alignof(Item) - 1) & ~(uintptr_t)(alignof(Item) - 1);
        return reinterpret_cast<Item*>(aligned_start);
    }

    static uint32_t get_capacity(Block *b) {
        uintptr_t block_end = reinterpret_cast<uintptr_t>(b) + BlockSize;
        uintptr_t header_end = reinterpret_cast<uintptr_t>(b) + sizeof(Block);
        uintptr_t aligned_start = (header_end + alignof(Item) - 1) & ~(uintptr_t)(alignof(Item) - 1);
        return (block_end - aligned_start) / sizeof(Item);
    }

    Block *m_head;
    Block *m_avail;

    void create_next_block() {
        void *block;
        posix_memalign(&block, BlockSize, BlockSize);

        Block *header = new (block) Block();
        header->next = m_head;
        header->next_avail = m_avail;
        header->first_free = 0;
        m_head = header;
        m_avail = header;

        Item *items = get_items(header);
        uint32_t cap = get_capacity(header);

        for (uint32_t i = 0; i < cap - 1; ++i)
            items[i].next = i + 1;
        items[cap - 1].next = UINT32_MAX;
    }
};

// ordered, arbitrary size allocator
template<size_t BlockSize = 65536>
class LinearAllocator {
public:
    LinearAllocator() : m_head(nullptr) {
        allocate_block();
    }

    ~LinearAllocator() {
        Block* b = m_head;
        while (b) {
            Block* next = b->next;
            ::free(b);
            b = next;
        }
    }

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    template<typename T, typename... Args>
    T* alloc(Args&&... args) {
        static_assert(sizeof(T) <= BlockSize - sizeof(Block), "Type too large for Arena block");
        static_assert(std::is_trivially_destructible<T>::value,
            "LinearAllocator only supports trivially destructible types to prevent memory leaks on destruction.");

        size_t alignment = alignof(T);

        uintptr_t curr_ptr = reinterpret_cast<uintptr_t>(m_head->curr);
        uintptr_t aligned_ptr = (curr_ptr + (alignment - 1)) & ~(uintptr_t)(alignment - 1);

        if (aligned_ptr + sizeof(T) > reinterpret_cast<uintptr_t>(m_head->end)) {
            allocate_block();
            curr_ptr = reinterpret_cast<uintptr_t>(m_head->curr);
            aligned_ptr = (curr_ptr + (alignment - 1)) & ~(uintptr_t)(alignment - 1);
        }

        m_head->curr = reinterpret_cast<uint8_t*>(aligned_ptr + sizeof(T));

        return new (reinterpret_cast<void*>(aligned_ptr)) T(std::forward<Args>(args)...);
    }

    void reset() {
        Block* b = m_head;
        while (b) {
            b->curr = b->data();
            b = b->next;
        }
    }

private:
    struct Block {
        Block* next;
        uint8_t* curr;
        uint8_t* end;

        uint8_t* data() { return reinterpret_cast<uint8_t*>(this + 1); }
    };

    Block* m_head;

    void allocate_block() {
        void* raw = std::malloc(BlockSize);

        Block* new_block = new (raw) Block();
        new_block->next = m_head;
        new_block->curr = new_block->data();
        new_block->end = reinterpret_cast<uint8_t*>(raw) + BlockSize;

        m_head = new_block;
    }
};
