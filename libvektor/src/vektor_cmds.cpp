#include "amdgpu/sdma_encoder.h"
#include "vektor/vektor.h"
#include "vektor_impl.h"

#include "beta.h"
#include <fmt/format.h>

namespace vektor {

void memset_transfer(CommandListImpl *impl, gpuptr_t addr, std::size_t size, uint32_t value) {
    assert(impl->queue->type == QueueType::Transfer, "memset_transfer: requires queue of Transfer type");

    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    while (size > 0) {
        uint64_t bytes_written = enc.constant_fill(addr, size, value);
        size -= bytes_written;
        addr += bytes_written;
    }
}

void memset(CommandList pcl, gpuptr_t addr, std::size_t size, uint32_t value) {
    auto *cl = (CommandListImpl *)pcl;
    assert(cl, "memset: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case QueueType::Transfer:
        memset_transfer(cl, addr, size, value);
        break;
    default:
        not_implemented("memset: not implemented for queue type: {}", cl->queue->type);
    }
}

SDMAAtomicOp sdma_atomic_op_map(Signal sig) {
    switch(sig) {
    case Signal::AtomicSet:
        return SDMAAtomicOp::Swap;
    case Signal::AtomicMax:
        return SDMAAtomicOp::UMax;
    case Signal::AtomicOr:
        return SDMAAtomicOp::Or;
    default:
    not_implemented("sdma_atomic_op_map: no mapping for {}", sig);
    }
}

SDMAWaitMemOp sdma_waitmem_op_map(Op op) {
    switch(op) {
    case Op::Equal:
        return SDMAWaitMemOp::Equal;
    default:
        not_implemented("sdma_waitmem_op_map: no mapping for {}", op);
    }
}

void wait_before_transfer(CommandListImpl *impl, gpuptr_t ptr, uint64_t value, Op op, uint64_t mask) {
    assert(impl->queue->type == QueueType::Transfer, "wait_before_transfer: requires queue of Transfer type");

    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    auto func = sdma_waitmem_op_map(op);

    // @todo: NOTE: this only writes the low 32-bits of value. I do not know how we should do this, it
    // seems the hardware doesn't support this.
    enc.wait_mem(func, ptr, value & 0xFFFFFFFF, mask & 0xFFFFFFFF);
}

void wait_before(CommandList pcl, Stage after, gpuptr_t ptr, uint64_t value, Op op, HazardFlags hazard, uint64_t mask) {
    auto *cl = (CommandListImpl *)pcl;
    assert(cl, "wait_before: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case QueueType::Transfer:
        wait_before_transfer(cl, ptr, value, op, mask);
        break;
    default:
        not_implemented("wait_before: not implemented for queue type: {}", cl->queue->type);
    }
}

void signal_after_transfer(CommandListImpl *impl, gpuptr_t ptr, uint64_t value, Signal sig) {
    assert(impl->queue->type == QueueType::Transfer, "signal_after_transfer: requires queue of Transfer type");
    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    auto op = sdma_atomic_op_map(sig);

    enc.atomic(op, ptr, value);
}

void signal_after(CommandList pcl, Stage before, gpuptr_t ptr, uint64_t value, Signal sig) {
    auto *cl = (CommandListImpl *)pcl;
    assert(cl, "signal_after: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case QueueType::Transfer:
        signal_after_transfer(cl, ptr, value, sig);
        break;
    default:
        not_implemented("wait_before: not implemented for queue type: {}", cl->queue->type);
    }
}

void write_timestamp_transfer(CommandListImpl *impl, gpuptr_t ptr) {
    assert(impl->queue->type == QueueType::Transfer, "write_timestamp_transfer: requires queue of Transfer type");
    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    enc.write_timestamp(ptr);
}

void write_timestamp(CommandList pcl, gpuptr_t ptr) {
    auto *cl = (CommandListImpl *)pcl;
    assert(cl, "write_timestamp: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case QueueType::Transfer:
        write_timestamp_transfer(cl, ptr);
        break;
    default:
        not_implemented("write_timestamp: not implemented for queue type: {}", cl->queue->type);
    }
}

}
