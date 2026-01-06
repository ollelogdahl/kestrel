#include "cp_encoder.h"
#include "cmdstream.h"
#include "gpuinfo.h"

CPEncoder::CPEncoder(GpuInfo &info, uint8_t ip_type, CommandStream &cs) : info(info), ip_type(ip_type), cs(cs) {}

void CPEncoder::wait_mem(WaitMemOp op, uint64_t va, uint32_t ref, uint32_t mask) {
    cs.emit(PKT3(PKT3_WAIT_REG_MEM, 5, 0));
    cs.emit(WAIT_REG_MEM_MEM_SPACE(1) | (uint32_t)op);
    cs.emit(va);
    cs.emit(va >> 32);
    cs.emit(ref);
    cs.emit(mask);
    cs.emit(4);
}

void CPEncoder::release_mem(uint32_t event,
    uint32_t event_flags, uint32_t dst_sel,
    uint32_t int_sel, uint32_t data_sel, uint64_t va,
    uint32_t new_fence) {

    assert(info.gfx_level >= GfxLevel::GFX12 || !event_flags || (event != V_028A90_PS_DONE &&
                                                    event != V_028A90_CS_DONE), "release_mem: only gfx12+ support gcr ops with PS_DONE & CS_DONE.");

    const uint32_t op = EVENT_TYPE(event) |
                        EVENT_INDEX(event == V_028A90_CS_DONE || event == V_028A90_PS_DONE ? 6 : 5) |
                        event_flags;
    const uint32_t sel = EOP_DST_SEL(dst_sel) |
                         EOP_INT_SEL(int_sel) |
                         EOP_DATA_SEL(data_sel);

    cs.emit(PKT3(PKT3_RELEASE_MEM, 6, false));
    cs.emit(op);
    cs.emit(sel);
    cs.emit(va);
    cs.emit(va >> 32);
    cs.emit(new_fence); /* immediate data lo */
    cs.emit(0);         /* immediate data hi */
    cs.emit(0);
}

void CPEncoder::atomic_mem(AtomicOp op, uint32_t atomic_cmd, uint64_t va, uint64_t data, uint64_t compare_data) {
    cs.emit(PKT3(PKT3_ATOMIC_MEM, 7, 0));
    cs.emit(ATOMIC_OP((uint32_t)op) |
                   ATOMIC_COMMAND(atomic_cmd));
    cs.emit(va);
    cs.emit(va >> 32);
    cs.emit(data);
    cs.emit(data >> 32);
    cs.emit(compare_data);
    cs.emit(compare_data >> 32);
    cs.emit(10);                    /* loop interval */
}

void CPEncoder::cond_exec(uint64_t va, uint32_t count) {
    cs.emit(PKT3(PKT3_COND_EXEC, 3, 0));
    cs.emit(va);
    cs.emit(va >> 32);
    cs.emit(0);
    cs.emit(count);
}
