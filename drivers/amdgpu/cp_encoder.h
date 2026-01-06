#pragma once

#include "cmdstream.h"
#include "gpuinfo.h"
#include "impl.h"
#include "pm4_encoder.h"

/**
 * Command Processor command encoder
 */
class CPEncoder {
public:
    CPEncoder(GpuInfo &info, uint8_t ip_type, CommandStream &cs);

    Pm4Encoder &pm4() { return m_pm4; }

    // nops are variable length; we can write data here if we want
    // (scratch space for example).
    void nop(uint32_t count = 1, uint32_t *content = nullptr);

    void wait_mem(WaitMemOp op, uint64_t va, uint32_t ref, uint32_t mask);

    void release_mem(uint32_t event,
        uint32_t event_flags, uint32_t dst_sel,
        uint32_t int_sel, uint32_t data_sel, uint64_t va,
        uint32_t new_fence);

    void atomic_mem(AtomicOp op, uint32_t atomic_cmd, uint64_t va, uint64_t data, uint64_t compare_data);

    void cond_exec(uint64_t va, uint32_t count);
private:
    GpuInfo &info;
    uint8_t ip_type;
    CommandStream &cs;

    Pm4Encoder m_pm4;
};
