#pragma once

#include <vector>
#include <cstdint>
#include <span>

class Pm4State {
public:
    static Pm4State create_sized(bool debug_sqtt,
        unsigned max_dw, bool is_compute_queue);

    void set_reg(unsigned reg, uint32_t val);
    void set_reg_custom(unsigned reg, uint32_t val,
                          unsigned opcode, unsigned idx);
    void set_reg_idx3(unsigned reg, uint32_t val);
    void clear_state(/* const struct radeon_info *info @todo, */
                       bool debug_sqtt, bool is_compute_queue);
    void cmd_begin(unsigned opcode);
    void cmd_add(uint32_t dw);
    void cmd_end(bool predicate);

    void finalize();
    void emit(CommandStream &cs);

private:
    uint16_t last_reg;   /* register offset in dwords */
    uint16_t last_pm4;
    uint16_t ndw;        /* number of dwords in pm4 */
    uint8_t last_opcode;
    uint8_t last_idx;
    bool is_compute_queue;
    bool packed_is_padded; /* whether SET_*_REG_PAIRS_PACKED is padded to an even number of regs */

    /* commands for the DE */
    uint16_t max_dw;

    /* Used by SQTT to override the shader address */
    bool debug_sqtt;
    uint32_t spi_shader_pgm_lo_reg;

    std::vector<uint32_t> pm4;
};
