#pragma once

#include <amdgpu.h>
#include <amdgpu_drm.h>

#include "amdgpu/gpuinfo.h"

struct DeviceImpl {
    int fd;
    amdgpu_device_handle amd_handle;

    GpuInfo info;
};
