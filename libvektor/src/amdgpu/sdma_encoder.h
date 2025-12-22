#pragma once

#include "cmdstream.h"
#include "gpuinfo.h"
#include <vector>
#include <span>

class SDMAEncoder {
public:
    SDMAEncoder(GpuInfo &info, CommandStream &cs);

    // returns the number of bytes written; may need to be repeated.
    uint64_t constant_fill(uint64_t va, uint64_t size, uint32_t value);
private:
    GpuInfo &info;
    CommandStream &cs;
};
