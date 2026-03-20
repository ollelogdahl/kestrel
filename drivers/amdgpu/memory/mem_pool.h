#pragma once

#include "impl.h"
#include <amdgpu.h>
#include <amdgpu_drm.h>

#include <cstddef>
#include <cstdint>

// @todo: come up with a better name.
// @todo: move out the pool concept outside; we may want to manage our own pools
//        sometimes (like one for shaders etc, not interlaced with user-allocations).
struct PoolAllocation {
    uint64_t gpu;
    void*    cpu;
    uint64_t size;
    void*    _internal;
};

PoolAllocation pool_alloc(DeviceImpl *dev,
                          size_t               size,
                          size_t               align,
                          uint32_t             domain,
                          uint32_t             flags);

void pool_free(void *internal);
