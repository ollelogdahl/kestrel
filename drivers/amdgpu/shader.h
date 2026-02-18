#pragma once

#include <cstdint>

struct ShaderRegs {
    uint32_t pgm_lo;
    uint32_t pgm_hi;
    uint32_t pgm_rsrc1;
    uint32_t pgm_rsrc2;
    uint32_t pgm_rsrc3;

    uint32_t userdata_0;
};

enum class HwStage {
    Compute
};

struct ShaderInfo {
    uint32_t block_size[3];
    HwStage hw_stage;
    ShaderRegs regs;

    bool ordered;
    uint32_t wave_size;
};

struct ShaderConfig {
    uint32_t pgm_rsrc1;
    uint32_t pgm_rsrc2;
    uint32_t pgm_rsrc3;
    uint32_t compute_resource_limits;

    uint32_t user_sgpr_count;
};

struct Shader {
    ShaderInfo info;
    ShaderConfig config;
    uint64_t va;

    KesAllocation allocation;
};
