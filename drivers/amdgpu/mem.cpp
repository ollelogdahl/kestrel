#include "impl.h"

#include "memory/mem_pool.h"

struct AllocationImpl {
    amdgpu_bo_handle bo;
    amdgpu_va_handle va_handle;
};

KesAllocation amdgpu_malloc(KesDevice pd, size_t size, size_t align, KesMemory memory) {
    auto *dev = reinterpret_cast<DeviceImpl *>(pd);

    int domain = AMDGPU_GEM_DOMAIN_VRAM;
    int flags = 0;

    switch(memory) {
    case KesMemoryDefault:
        flags |= AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
                    AMDGPU_GEM_CREATE_VRAM_CLEARED;
        break;
    case KesMemoryGpu:
        flags |= AMDGPU_GEM_CREATE_NO_CPU_ACCESS;
        break;
    case KesMemoryReadback:
        flags |= AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
                    AMDGPU_GEM_CREATE_COHERENT;
        break;
    }

    // @todo: some systems (DCE) require contiguous addresses as they don't use the MMU Infinity Cache.
    // req.flags |= AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS;

    auto res = pool_alloc(dev, size, align, domain, flags);

    KesAllocation alloc = {};
    if (res.gpu) {
        alloc.size = res.size;
        alloc.gpu = res.gpu;
        alloc.cpu = res.cpu;
        alloc._internal = res._internal;
    }

    return alloc;
}

void amdgpu_free(KesDevice pd, struct KesAllocation *alloc) {
    pool_free(alloc->_internal);

    alloc->gpu = 0;
    alloc->cpu = 0;
}
