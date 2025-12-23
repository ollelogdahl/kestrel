#pragma once

#include <cstdint>

#include "amdgpu_drm.h"

enum class GfxLevel {
    GFX9,
    GFX10,
    GFX10_3
};

#define SDMA_VERSION_VALUE(major, minor) (((major) << 8) | (minor))

enum class SDMAVersion {
    /* GFX10.3 */
    SDMA_5_2 = SDMA_VERSION_VALUE(5, 2),
    /* GFX11 */
    SDMA_6_0 = SDMA_VERSION_VALUE(6, 0),
};

struct IpInfo {
    uint8_t ver_major;
    uint8_t ver_minor;
    uint8_t ver_rev;
    uint8_t num_queues;
    uint8_t num_instances;
    uint32_t ib_alignment;
    uint32_t ib_pad_dw_mask;
};

struct GpuInfo {
    GfxLevel gfx_level;
    uint32_t address32_hi;
    SDMAVersion sdma_version;

    IpInfo ip[AMDGPU_HW_IP_NUM];
};
