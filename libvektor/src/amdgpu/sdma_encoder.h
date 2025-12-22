#pragma once

#include "cmdstream.h"
#include "gpuinfo.h"
#include <vector>
#include <span>

class SDMAEncoder {
public:
    SDMAEncoder(GpuInfo &info, CommandStream &cs);

    void write_timestamp(uint64_t va);
    void semaphore(uint64_t va);
    void fence(uint64_t va, uint32_t fence);
    void wait_mem(uint32_t op, uint64_t va, uint32_t ref, uint32_t mask);

    // returns the number of bytes written; may need to be repeated.
    uint64_t constant_fill(uint64_t va, uint64_t size, uint32_t value);
private:
    GpuInfo &info;
    CommandStream &cs;
};
