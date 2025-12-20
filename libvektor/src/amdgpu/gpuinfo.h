#pragma once

#include <cstdint>

enum class GfxLevel {
    GFX9,
    GFX10,
    GFX10_3
};

struct GpuInfo {
    GfxLevel gfx_level;
    uint32_t address32_hi;
};
