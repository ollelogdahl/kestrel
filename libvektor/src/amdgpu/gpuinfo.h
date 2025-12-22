#pragma once

#include <cstdint>

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

struct GpuInfo {
    GfxLevel gfx_level;
    uint32_t address32_hi;
    SDMAVersion sdma_version;
};
