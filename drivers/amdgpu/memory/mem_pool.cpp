#include "mem_pool.h"
#include "tlsf.h"

#include <cassert>
#include <cstring>
#include <mutex>
#include <vector>
#include <unordered_map>

// Allocations larger than SLAB_SIZE get a dedicated BO with no sub-allocator.
static constexpr uint64_t SLAB_SIZE        = 64ull * 1024 * 1024;
static constexpr uint64_t MIN_VA_ALIGNMENT = 4096;

struct Pool;
struct Slab;

struct Slab {
    Pool*                pool      = nullptr;
    DeviceImpl *dev       = nullptr;
    amdgpu_bo_handle     bo        = nullptr;
    amdgpu_va_handle     va_handle = nullptr;
    uint64_t             gpu_base  = 0;
    void*                cpu_base  = nullptr;
    uint64_t             size      = 0;
    TlsfAllocator*       tlsf      = nullptr;  // null for dedicated slabs
    bool                 dedicated = false;
};

struct AllocationImpl {
    Slab*                     slab;
    TlsfAllocator::Allocation tlsf;   // unused for dedicated slabs
};

struct PoolKey {
    uint32_t domain;
    uint32_t flags;
    bool operator==(const PoolKey& o) const {
        return domain == o.domain && flags == o.flags;
    }
};

struct PoolKeyHash {
    size_t operator()(const PoolKey& k) const {
        return std::hash<uint64_t>()((uint64_t)k.domain << 32 | k.flags);
    }
};

struct Pool {
    PoolKey            key;
    std::mutex         mutex;
    std::vector<Slab*> slabs; // pooled slabs only; dedicated slabs are not listed
};

static std::mutex g_registry_mutex;
static std::unordered_map<PoolKey, Pool*, PoolKeyHash> g_pools;

static Pool* get_or_create_pool(const PoolKey& key) {
    std::lock_guard<std::mutex> lk(g_registry_mutex);
    auto it = g_pools.find(key);
    if (it != g_pools.end()) return it->second;

    Pool* pool   = new Pool;
    pool->key    = key;
    g_pools[key] = pool;
    return pool;
}

#include <iostream>

static Slab* slab_create(Pool* pool,
                          DeviceImpl *dev,
                          uint64_t size,
                          uint32_t domain,
                          uint32_t flags,
                          bool dedicated) {

    std::cout << "create slab " << std::endl;

    amdgpu_bo_alloc_request req = {};
    req.alloc_size     = size;
    req.phys_alignment = MIN_VA_ALIGNMENT;
    req.preferred_heap = domain;
    req.flags          = flags;

    amdgpu_bo_handle bo = nullptr;
    if (amdgpu_bo_alloc(dev->amd_handle, &req, &bo) != 0)
        return nullptr;

    uint64_t         gpu_base  = 0;
    amdgpu_va_handle va_handle = nullptr;
    if (amdgpu_va_range_alloc(dev->amd_handle,
                               amdgpu_gpu_va_range_general,
                               size, MIN_VA_ALIGNMENT, 0,
                               &gpu_base, &va_handle, 0) != 0) {
        amdgpu_bo_free(bo);
        return nullptr;
    }

    const uint32_t va_flags = AMDGPU_VM_PAGE_READABLE  |
                              AMDGPU_VM_PAGE_WRITEABLE  |
                              AMDGPU_VM_PAGE_EXECUTABLE;
    if (amdgpu_bo_va_op(bo, 0, size, gpu_base, va_flags, AMDGPU_VA_OP_MAP) != 0) {
        amdgpu_va_range_free(va_handle);
        amdgpu_bo_free(bo);
        return nullptr;
    }

    void* cpu_base = nullptr;
    if (!(flags & AMDGPU_GEM_CREATE_NO_CPU_ACCESS)) {
        if (amdgpu_bo_cpu_map(bo, &cpu_base) != 0) {
            amdgpu_bo_va_op(bo, 0, size, gpu_base, 0, AMDGPU_VA_OP_UNMAP);
            amdgpu_va_range_free(va_handle);
            amdgpu_bo_free(bo);
            return nullptr;
        }
    }

    device_register_allocation(dev, bo);

    Slab* slab       = new Slab;
    slab->pool       = pool;
    slab->dev        = dev;
    slab->bo         = bo;
    slab->va_handle  = va_handle;
    slab->gpu_base   = gpu_base;
    slab->cpu_base   = cpu_base;
    slab->size       = size;
    slab->dedicated  = dedicated;
    slab->tlsf       = dedicated ? nullptr : new TlsfAllocator(size);
    return slab;
}

static void slab_destroy(Slab* slab) {
    if (slab->cpu_base)
        amdgpu_bo_cpu_unmap(slab->bo);

    amdgpu_bo_va_op(slab->bo, 0, slab->size,
                    slab->gpu_base, 0, AMDGPU_VA_OP_UNMAP);
    amdgpu_va_range_free(slab->va_handle);
    amdgpu_bo_free(slab->bo);

    delete slab->tlsf;
    delete slab;
}

PoolAllocation pool_alloc(DeviceImpl *dev,
                          size_t size,
                          size_t align,
                          uint32_t domain,
                          uint32_t flags) {
    PoolAllocation out = {};
    if (!dev || size == 0) return out;

    assert((align & (align - 1)) == 0 && "alignment must be a power of two");

    Pool* pool = get_or_create_pool({ domain, flags });

    // Large allocations bypass the pool entirely: one BO, one allocation, no
    // TLSF overhead.  We still need the pool pointer for the mutex on free.
    if ((uint64_t)size > SLAB_SIZE) {
        Slab* slab = slab_create(pool, dev, (uint64_t)size, domain, flags,
                                  true);
        if (!slab) return out;

        AllocationImpl* impl = new AllocationImpl;
        impl->slab = slab;

        out.gpu       = slab->gpu_base;
        out.cpu       = slab->cpu_base;
        out.size      = slab->size;
        out._internal = impl;
        return out;
    }

    std::lock_guard<std::mutex> lk(pool->mutex);

    TlsfAllocator::Allocation tlsf;
    Slab* chosen = nullptr;

    // Try existing pooled slabs, newest first.
    for (int i = (int)pool->slabs.size() - 1; i >= 0; --i) {
        if (pool->slabs[i]->tlsf->allocate(size, align, tlsf)) {
            chosen = pool->slabs[i];
            break;
        }
    }

    if (!chosen) {
        Slab* slab = slab_create(pool, dev, SLAB_SIZE, domain, flags, false);
        if (!slab) return out;

        pool->slabs.push_back(slab);

        if (!slab->tlsf->allocate(size, align, tlsf)) {
            assert(false && "fresh slab could not satisfy allocation");
            return out;
        }
        chosen = slab;
    }

    AllocationImpl* impl = new AllocationImpl{ chosen, tlsf };

    out.gpu       = chosen->gpu_base + tlsf.offset;
    out.cpu       = chosen->cpu_base
                  ? (uint8_t*)chosen->cpu_base + tlsf.offset
                  : nullptr;
    out.size      = tlsf.size;
    out._internal = impl;
    return out;
}

void pool_free(void *internal)
{
    if (!internal) return;

    AllocationImpl* impl = static_cast<AllocationImpl*>(internal);
    Slab*           slab = impl->slab;

    if (slab->dedicated) {
        // No pool lock needed — dedicated slabs are not shared.
        // Destroy the slab immediately; its only allocation is gone.
        slab_destroy(slab);
    } else {
        Pool* pool = slab->pool;
        std::lock_guard<std::mutex> lk(pool->mutex);
        slab->tlsf->free(impl->tlsf);
    }

    delete impl;
}
