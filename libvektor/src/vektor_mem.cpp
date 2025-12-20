#include "vektor/vektor.h"
#include "vektor_impl.h"

#include "beta.h"

struct AllocationImpl {
    amdgpu_bo_handle bo;
    amdgpu_va_handle va_handle;
};

#define VEK_HUGE_PAGE_SIZE (2ULL * 1024 * 1024)

namespace vektor {

Allocation malloc(Device pd, std::size_t size, Memory memory) {
    return malloc(pd, size, VEK_HUGE_PAGE_SIZE, memory);
}

Allocation malloc(Device pd, size_t size, size_t align, Memory memory) {
    auto *dev = (DeviceImpl *)pd;

    auto aligned_size = (size + VEK_HUGE_PAGE_SIZE - 1) & ~(VEK_HUGE_PAGE_SIZE - 1);
    auto alignment = VEK_HUGE_PAGE_SIZE;

    Allocation alloc = {};
    AllocationImpl *impl = reinterpret_cast<AllocationImpl*>(alloc._impl);

    alloc.size = aligned_size;
    impl->bo = nullptr;
    impl->va_handle = nullptr;

    amdgpu_bo_alloc_request req = {};
    req.alloc_size = aligned_size;
    req.phys_alignment = alignment;
    req.preferred_heap = AMDGPU_GEM_DOMAIN_VRAM;

    switch(memory) {
    case Memory::Default:
        req.flags = AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
                    AMDGPU_GEM_CREATE_VRAM_CLEARED;
        break;
    case Memory::Gpu:
        req.flags = AMDGPU_GEM_CREATE_NO_CPU_ACCESS;
        break;
    case Memory::Readback:
        req.flags = AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
                    AMDGPU_GEM_CREATE_COHERENT;
        break;
    }

    // some systems (DCE) require contiguous addresses as they don't use the MMU Infinity Cache.
    // req.flags |= AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS;

    log("bo alloc: size: {} align: {}", aligned_size, alignment);

    if (amdgpu_bo_alloc(dev->amd_handle, &req, &impl->bo) != 0) {
        log("amdgpu_bo_alloc failed");
        return alloc;
    }

    uint64_t va_base;
    int r = amdgpu_va_range_alloc(dev->amd_handle,
        amdgpu_gpu_va_range_general,
        aligned_size, alignment, 0,
        &va_base, &impl->va_handle, 0);
    if (r != 0) {
        log("amdgpu_va_range_alloc failed");
        amdgpu_bo_free(impl->bo);
        impl->bo = nullptr;
        return alloc;
    }
    alloc.gpu = va_base;

    auto va_flags = AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE | AMDGPU_VM_PAGE_EXECUTABLE;
    r = amdgpu_bo_va_op(impl->bo, 0, aligned_size, va_base, va_flags, AMDGPU_VA_OP_MAP);
    if (r != 0) {
        log("amdgpu_bo_va_op failed");
        amdgpu_va_range_free(impl->va_handle);
        amdgpu_bo_free(impl->bo);
        impl->bo = nullptr;
        impl->va_handle = nullptr;
        return alloc;
    }

    if (memory != Memory::Gpu) {
        r = amdgpu_bo_cpu_map(impl->bo, &alloc.cpu);
        if (r != 0) {
            log("amdgpu_bo_cpu_map failed");
            amdgpu_bo_va_op(impl->bo, 0, aligned_size, va_base, 0, AMDGPU_VA_OP_UNMAP);
            amdgpu_va_range_free(impl->va_handle);
            amdgpu_bo_free(impl->bo);
            impl->bo = nullptr;
            impl->va_handle = nullptr;
            alloc.gpu = 0;
            return alloc;
        }
    }

    return alloc;
}

void free(Device pd, Allocation &alloc) {
    AllocationImpl *impl = reinterpret_cast<AllocationImpl*>(alloc._impl);

    if (alloc.cpu) {
        amdgpu_bo_cpu_unmap(impl->bo);
        alloc.cpu = nullptr;
    }
    if (impl->bo) {
        amdgpu_bo_va_op(impl->bo, 0, alloc.size, alloc.gpu, 0, AMDGPU_VA_OP_UNMAP);
        amdgpu_bo_free(impl->bo);
        impl->bo = nullptr;
    }
    if (impl->va_handle) {
        amdgpu_va_range_free(impl->va_handle);
        impl->va_handle = nullptr;
    }

    alloc.gpu = 0;
}

}
