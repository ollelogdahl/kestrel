#include "sdma_encoder.h"

#include "cmdstream.h"
#include "sid.h"

SDMAEncoder::SDMAEncoder(GpuInfo &info, CommandStream &cs) : info(info), cs(cs) {}

#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )

#define BITFIELD64_BIT(b)      (uint64_t(1) << (b))

#define BITFIELD64_MASK(b)      \
   ((b) == 64 ? (~uint64_t(0)) : BITFIELD64_BIT((b) & 63) - 1)

void SDMAEncoder::write_timestamp(uint64_t va) {
    SDMA_OPCODE_SEMAPHORE
    cs.emit(SDMA_PACKET(SDMA_OPCODE_TIMESTAMP, SDMA_TS_SUB_OPCODE_GET_GLOBAL_TIMESTAMP, 0));
    cs.emit(va);
    cs.emit(va >> 32);
}

// @todo: taken from various drivers online... not mesa, weird!
void SDMAEncoder::semaphore(uint64_t va) {
    cs.emit(SDMA_PACKET(SDMA_OPCODE_SEMAPHORE, 0, 0));
    cs.emit(va);
    cs.emit(va >> 32);
}

void SDMAEncoder::fence(uint64_t va, uint32_t fence) {
    cs.emit(SDMA_PACKET(SDMA_OPCODE_FENCE, 0, SDMA_FENCE_MTYPE_UC));
    cs.emit(va);
    cs.emit(va >> 32);
    cs.emit(fence);
}

void SDMAEncoder::wait_mem(uint32_t op, uint64_t va, uint32_t ref, uint32_t mask) {
    cs.emit(SDMA_PACKET(SDMA_OPCODE_POLL_REGMEM, 0, 0) | op << 28 | SDMA_POLL_MEM);
    cs.emit(va);
    cs.emit(va >> 32);
    cs.emit(ref);
    cs.emit(mask);
    cs.emit(SDMA_POLL_INTERVAL_160_CLK | SDMA_POLL_RETRY_INDEFINITELY << 16);
}

uint64_t SDMAEncoder::constant_fill(uint64_t va, uint64_t size, uint32_t value) {
    const uint32_t fill_size = 2;
    const uint64_t max_fill_size = BITFIELD64_MASK(info.sdma_version >= SDMAVersion::SDMA_6_0 ? 30 : 22) & ~0x3;
    const uint64_t bytes_written = MIN2(size, max_fill_size);

    cs.emit(SDMA_PACKET(SDMA_OPCODE_CONSTANT_FILL, 0, 0) | (fill_size << 30));
    cs.emit(va);
    cs.emit(va >> 32);
    cs.emit(value);
    cs.emit(bytes_written - 1);

    return bytes_written;
}
