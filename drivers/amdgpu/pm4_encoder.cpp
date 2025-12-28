#include "pm4_encoder.h"
#include "beta.h"


#include "gpuinfo.h"
#include "amdgfxregs.h"
#include "sid.h"
#include <amdgpu_drm.h>

Pm4Encoder::Pm4Encoder(GpuInfo &info, uint8_t ip_type) : info(info), ip_type(ip_type) {

}

void Pm4Encoder::emit(uint32_t value) {
    buf.push_back(value);
}

void Pm4Encoder::emit(std::span<uint32_t> values) {
    buf.insert(buf.end(), values.begin(), values.end());
}

void Pm4Encoder::set_reg_seq(uint32_t reg, uint32_t num, uint32_t idx, uint32_t bank_offset, uint32_t bank_end, uint32_t packet, uint32_t reset_filter_cam) {
    assert(reg >= bank_offset && reg < bank_end, "register out of range: {}", reg);
    emit(PKT3(packet, num, 0) | PKT3_RESET_FILTER_CAM_S(reset_filter_cam));
    emit(((reg - bank_offset) >> 2) | (idx << 28));
}

void Pm4Encoder::set_reg(uint32_t reg, uint32_t idx, uint32_t value, uint32_t bank_offset, uint32_t bank_end, uint32_t packet) {
    set_reg_seq(reg, 1, idx, bank_offset, bank_end, packet, 0);
    emit(value);
}

void Pm4Encoder::set_config_reg_seq(uint32_t reg, uint32_t num) {
    set_reg_seq(reg, num, 0, SI_CONFIG_REG_OFFSET, SI_CONFIG_REG_END, PKT3_SET_CONFIG_REG, 0);
}

void Pm4Encoder::set_config_reg(uint32_t reg, uint32_t value) {
    set_reg(reg, 0, value, SI_CONFIG_REG_OFFSET, SI_CONFIG_REG_END, PKT3_SET_CONFIG_REG);
}

void Pm4Encoder::set_uconfig_reg_seq(uint32_t reg, uint32_t num) {
    set_reg_seq(reg, num, 0, CIK_UCONFIG_REG_OFFSET, CIK_UCONFIG_REG_END, PKT3_SET_UCONFIG_REG, 0);
}

void Pm4Encoder::set_uconfig_reg(uint32_t reg, uint32_t value) {
    set_reg(reg, 0, value, CIK_UCONFIG_REG_OFFSET, CIK_UCONFIG_REG_END, PKT3_SET_UCONFIG_REG);
}

void Pm4Encoder::set_uconfig_reg_idx(uint32_t reg, uint32_t idx, uint32_t value) {
    set_reg(reg, idx, value, CIK_UCONFIG_REG_OFFSET, CIK_UCONFIG_REG_END, PKT3_SET_UCONFIG_REG_INDEX);
}

/*
 * On GFX10, there is a bug with the ME implementation of its content
 * addressable memory (CAM), that means that it can skip register writes due
 * to not taking correctly into account the fields from the GRBM_GFX_INDEX.
 * With this __filter_cam_workaround bit we can force the write.
 */
void Pm4Encoder::set_uconfig_perfctr_reg_seq(uint32_t reg, uint32_t num) {
    bool filter_cam_workaround = (info.gfx_level > GfxLevel::GFX10) && ip_type == AMDGPU_HW_IP_GFX;
    set_reg_seq(reg, num, 0, CIK_UCONFIG_REG_OFFSET, CIK_UCONFIG_REG_END, PKT3_SET_UCONFIG_REG, filter_cam_workaround);
}

void Pm4Encoder::set_uconfig_perfctr_reg(uint32_t reg, uint32_t value) {
    set_uconfig_perfctr_reg_seq(reg, 1);
    emit(value);
}

void Pm4Encoder::set_context_reg_seq(uint32_t reg, uint32_t num) {
    set_reg_seq(reg, num, 0, SI_CONTEXT_REG_OFFSET, SI_CONTEXT_REG_END, PKT3_SET_CONTEXT_REG, 0);
}

void Pm4Encoder::set_context_reg(uint32_t reg, uint32_t value) {
    set_reg(reg, 0, value, SI_CONTEXT_REG_OFFSET, SI_CONTEXT_REG_END, PKT3_SET_CONTEXT_REG);
}

void Pm4Encoder::set_context_reg_idx(uint32_t reg, uint32_t idx, uint32_t value) {
    set_reg(reg, idx, value, SI_CONTEXT_REG_OFFSET, SI_CONTEXT_REG_END, PKT3_SET_CONTEXT_REG);
}

void Pm4Encoder::set_sh_reg_seq(uint32_t reg, uint32_t num) {
    set_reg_seq(reg, num, 0, SI_SH_REG_OFFSET, SI_SH_REG_END, PKT3_SET_SH_REG, 0);
}

void Pm4Encoder::set_sh_reg(uint32_t reg, uint32_t value) {
    set_reg(reg, 0, value, SI_SH_REG_OFFSET, SI_SH_REG_END, PKT3_SET_SH_REG);
}

void Pm4Encoder::set_sh_reg_idx(uint32_t reg, uint32_t idx, uint32_t value) {
    uint32_t opcode = PKT3_SET_SH_REG_INDEX;
    set_reg(reg, idx, value, SI_SH_REG_OFFSET, SI_SH_REG_END, opcode);
}

void Pm4Encoder::emit_32bit_pointer(uint32_t sh_offset, uint64_t va) {
    assert(va == 0 || (va >> 32) == info.address32_hi, "va outside valid range: {}", va);
    set_sh_reg(sh_offset, va);
}

void Pm4Encoder::emit_64bit_pointer(uint32_t sh_offset, uint64_t va) {
    set_sh_reg(sh_offset, 2);
    emit(va);
    emit(va >> 32);
}

void Pm4Encoder::event_write_predicate(uint32_t event_type, bool predicate) {
    emit(PKT3(PKT3_EVENT_WRITE, 0, predicate));
    auto ev_index = event_type == V_028A90_VS_PARTIAL_FLUSH ||
                event_type == V_028A90_PS_PARTIAL_FLUSH ||
                event_type == V_028A90_CS_PARTIAL_FLUSH ? 4 :
                event_type == V_028A90_PIXEL_PIPE_STAT_CONTROL ? 1 : 0;
    emit(EVENT_TYPE(event_type) | EVENT_INDEX(ev_index));
}

void Pm4Encoder::event_write(uint32_t event_type) {
    event_write_predicate(event_type, false);
}

void Pm4Encoder::set_privileged_config_reg(uint32_t reg, uint32_t value) {
    assert(reg < CIK_UCONFIG_REG_OFFSET, "reg outside valid range for privileged config: {}", reg);
    emit(PKT3(PKT3_COPY_DATA, 4, 0));
    emit(COPY_DATA_SRC_SEL(COPY_DATA_IMM) | COPY_DATA_DST_SEL(COPY_DATA_PERF));
    emit(value);
    emit(0);
    emit(reg >> 2);
    emit(0);
}
