#pragma once

#include "vektor/vektor.h"

#include <amdgpu.h>
#include <amdgpu_drm.h>

#include "amdgpu/cmdstream.h"
#include "amdgpu/gpuinfo.h"

namespace vektor {

struct DeviceImpl {
    int fd;
    amdgpu_device_handle amd_handle;

    GpuInfo info;
};

struct QueueImpl {
    amdgpu_context_handle ctx_handle;
    DeviceImpl *dev;
    QueueType type;

    uint32_t hw_ip_type;

    CommandRing *cmd_ring;
};

struct CommandListImpl {
    QueueImpl *queue;
    CommandStream cs;
};

}
