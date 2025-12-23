#pragma once

#include "cmdstream.h"
#include "gpuinfo.h"
#include <vector>
#include <span>

enum class SDMAAtomicOp {
    Swap = 0x67,
    Add = 0x6f,
    Sub = 0x70,
    UMin = 0x72,
    UMax = 0x74,
    Or = 0x76,
};

enum class SDMAWaitMemOp {
    Equal = 0x3
};

class SDMAEncoder {
public:
    SDMAEncoder(GpuInfo &info, CommandStream &cs);

    void write_timestamp(uint64_t va);
    void semaphore(uint64_t va);
    void fence(uint64_t va, uint32_t fence);
    void atomic(SDMAAtomicOp op, uint64_t va, uint64_t value);
    void wait_mem(SDMAWaitMemOp op, uint64_t va, uint32_t ref, uint32_t mask);

    // returns the number of bytes written; may need to be repeated.
    uint64_t constant_fill(uint64_t va, uint64_t size, uint32_t value);
private:
    GpuInfo &info;
    CommandStream &cs;
};
