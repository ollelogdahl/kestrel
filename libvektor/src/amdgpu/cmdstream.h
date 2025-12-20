#pragma once

#include <span>
#include <vector>

#include "gpuinfo.h"
// #include "pm4.h"

class CommandStream {
public:
    CommandStream(GpuInfo &info, uint8_t ip_type);

    void emit(uint32_t value);
    void emit(std::span<uint32_t> values);

    void emit_32bit_pointer(uint32_t sh_offset, uint64_t va);
    void emit_64bit_pointer(uint32_t sh_offset, uint64_t va);

    // @todo: figure out better sizes for reg, num, value, ...

    // Packet building helpers for CONFIG registers.
    void set_config_reg_seq(uint32_t reg, uint32_t num);
    void set_config_reg(uint32_t reg, uint32_t value);

    // Packet building helpers for UCONFIG registers.
    void set_uconfig_reg_seq(uint32_t reg, uint32_t num);
    void set_uconfig_reg(uint32_t reg, uint32_t value);
    void set_uconfig_reg_idx(uint32_t reg, uint32_t idx, uint32_t value);

    void set_uconfig_perfctr_reg_seq(uint32_t reg, uint32_t num);
    void set_uconfig_perfctr_reg(uint32_t reg, uint32_t value);

    // Packet building helpers for CONTEXT registers.
    void set_context_reg_seq(uint32_t reg, uint32_t num);
    void set_context_reg(uint32_t reg, uint32_t value);
    void set_context_reg_idx(uint32_t reg, uint32_t idx, uint32_t value);

    // Packet building helpers for SH registers.
    void set_sh_reg_seq(uint32_t reg, uint32_t num);
    void set_sh_reg(uint32_t reg, uint32_t value);
    void set_sh_reg_idx(uint32_t reg, uint32_t idx, uint32_t value);

    void event_write_predicate(uint32_t event_type, bool predicate);
    void event_write(uint32_t event_type);

    void set_privileged_config_reg(uint32_t reg, uint32_t value);
private:
    void set_reg_seq(uint32_t reg, uint32_t num, uint32_t idx, uint32_t bank_offset, uint32_t bank_end, uint32_t packet, uint32_t reset_filter_cam);
    void set_reg(uint32_t reg, uint32_t idx, uint32_t value, uint32_t bank_offset, uint32_t bank_end, uint32_t packet);

    GpuInfo &info;
    uint8_t ip_type;

    std::vector<uint32_t> buf;
    bool context_roll;
};
