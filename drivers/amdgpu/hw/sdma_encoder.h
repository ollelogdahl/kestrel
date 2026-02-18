#pragma once

#include "cmdstream.h"
#include "gpuinfo.h"
#include <vector>
#include <span>

#include "sid.h"

class SDMAEncoder {
public:
    SDMAEncoder(GpuInfo &info, CommandStream &cs);

    void write_timestamp(uint64_t va);
    void semaphore(uint64_t va);
    void fence(uint64_t va, uint32_t fence);
    void atomic(AtomicOp op, uint64_t va, uint64_t value);
    void wait_mem(WaitMemOp op, uint64_t va, uint32_t ref, uint32_t mask);

    // returns the number of bytes written; may need to be repeated.
    uint64_t constant_fill(uint64_t va, uint64_t size, uint32_t value);
    uint64_t copy_linear(uint64_t src_va, uint64_t dst_va, uint64_t size, bool tmz);
private:
    GpuInfo &info;
    CommandStream &cs;
};
