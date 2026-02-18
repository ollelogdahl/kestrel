#include "sdma_encoder.h"

#include "cmdstream.h"
#include "sid.h"

SDMAEncoder::SDMAEncoder(GpuInfo &info, CommandStream &cs) : info(info), cs(cs) {}

#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )

#define BITFIELD64_BIT(b)      (uint64_t(1) << (b))

#define BITFIELD64_MASK(b)      \
   ((b) == 64 ? (~uint64_t(0)) : BITFIELD64_BIT((b) & 63) - 1)

void SDMAEncoder::write_timestamp(uint64_t va) {
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

#define SDMA_OPCODE_ATOMIC 0xa

#define SDMA_L2_POLICY_LRU 0
#define SDMA_L2_POLICY_STREAM 1
#define SDMA_L2_POLICY_UC 3
#define SDMA_LLC_POLICY_MALL 0
#define SDMA_LLC_POLICY_BYPASS 1
#define SDMA_CACHE_POLICY(l2, llc) ((uint32_t)(llc) << 2 | (uint32_t)(l2))

#define SDMA_POLL_CACHE_POLICY(x) ((uint32_t)(x) << 20)
#define SDMA_POLL_CPV (1u << 24)
#define SDMA_POLL_HDP_FLUSH (1u << 26)

#define SDMA_ATOMIC_OP(x) ((uint32_t)(x) << 25)
#define SDMA_ATOMIC_CACHE_POLICY(x) ((uint32_t)(x) << 20)
#define SDMA_ATOMIC_CPV (1u << 24)

void SDMAEncoder::atomic(AtomicOp op, uint64_t va, uint64_t value) {
    uint32_t cache_policy = SDMA_CACHE_POLICY(SDMA_L2_POLICY_UC, SDMA_LLC_POLICY_BYPASS);
    cs.emit(SDMA_PACKET(SDMA_OPCODE_ATOMIC, 0, 0) | SDMA_ATOMIC_CPV | SDMA_ATOMIC_CACHE_POLICY(cache_policy) | SDMA_ATOMIC_OP(op));
    cs.emit(va);
    cs.emit(va >> 32);
    cs.emit(value);
    cs.emit(value >> 32);
    cs.emit(0);
    cs.emit(0);
    cs.emit(0);
}

void SDMAEncoder::wait_mem(WaitMemOp op, uint64_t va, uint32_t ref, uint32_t mask) {
    uint32_t cache_policy = SDMA_CACHE_POLICY(SDMA_L2_POLICY_UC, SDMA_LLC_POLICY_BYPASS);
    cs.emit(
        SDMA_PACKET(SDMA_OPCODE_POLL_REGMEM, 0, 0) | (uint32_t)op << 28 | SDMA_POLL_MEM
        | SDMA_POLL_HDP_FLUSH | SDMA_POLL_CPV | SDMA_POLL_CACHE_POLICY(cache_policy));
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

uint64_t SDMAEncoder::copy_linear(uint64_t src_va, uint64_t dst_va, uint64_t size, bool tmz) {
    const unsigned max_size_per_packet =
       info.sdma_version >= SDMAVersion::SDMA_5_2 ? SDMA_V5_2_COPY_MAX_BYTES : SDMA_V2_0_COPY_MAX_BYTES;
    uint32_t align = ~0u;

    /* SDMA FW automatically enables a faster dword copy mode when
     * source, destination and size are all dword-aligned.
     *
     * When source and destination are dword-aligned, round down the size to
     * take advantage of faster dword copy, and copy the remaining few bytes
     * with the last copy packet.
     */
    if ((src_va & 0x3) == 0 && (dst_va & 0x3) == 0 && size > 4 && (size & 0x3) != 0) {
       align = ~0x3u;
    }

    const uint64_t bytes_written = size >= 4 ? MIN2(size & align, max_size_per_packet) : size;

    cs.emit(SDMA_PACKET(SDMA_OPCODE_COPY, SDMA_COPY_SUB_OPCODE_LINEAR, (tmz ? 4 : 0)));
    cs.emit(info.sdma_version >= SDMAVersion::SDMA_4_0 ? bytes_written - 1 : bytes_written);
    cs.emit(0);
    cs.emit(src_va);
    cs.emit(src_va >> 32);
    cs.emit(dst_va);
    cs.emit(dst_va >> 32);

    return bytes_written;
}
