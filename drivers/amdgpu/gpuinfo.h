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
    SDMA_UNKNOWN = 0,
    /* GFX6 */
    SDMA_1_0 = SDMA_VERSION_VALUE(1, 0),

    /* GFX7 */
    SDMA_2_0 = SDMA_VERSION_VALUE(2, 0),

    /* GFX8 */
    SDMA_2_4 = SDMA_VERSION_VALUE(2, 4),
    SDMA_3_0 = SDMA_VERSION_VALUE(3, 0),
    SDMA_3_1 = SDMA_VERSION_VALUE(3, 1),

    /* GFX9 */
    SDMA_4_0 = SDMA_VERSION_VALUE(4, 0),
    SDMA_4_1 = SDMA_VERSION_VALUE(4, 1),
    SDMA_4_2 = SDMA_VERSION_VALUE(4, 2),
    SDMA_4_4 = SDMA_VERSION_VALUE(4, 4),

    /* GFX10 */
    SDMA_5_0 = SDMA_VERSION_VALUE(5, 0),

    /* GFX10.3 */
    SDMA_5_2 = SDMA_VERSION_VALUE(5, 2),

    /* GFX11 */
    SDMA_6_0 = SDMA_VERSION_VALUE(6, 0),

    /* GFX11.5 */
    SDMA_6_1 = SDMA_VERSION_VALUE(6, 1),

    /* GFX12 */
    SDMA_7_0 = SDMA_VERSION_VALUE(7, 0),
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
