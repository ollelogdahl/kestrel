#include "kestrel/kestrel.h"
#include "impl.h"

#include "sdma_encoder.h"

void memset_transfer(CommandListImpl *impl, kes_gpuptr_t addr, std::size_t size, uint32_t value) {
    assert(impl->queue->type == KesQueueTypeTransfer, "memset_transfer: requires queue of Transfer type");

    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    while (size > 0) {
        uint64_t bytes_written = enc.constant_fill(addr, size, value);
        size -= bytes_written;
        addr += bytes_written;
    }
}

void amdgpu_cmd_memset(KesCommandList pcl, kes_gpuptr_t addr, std::size_t size, uint32_t value) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "memset: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case KesQueueTypeTransfer:
        memset_transfer(cl, addr, size, value);
        break;
    default:
        not_implemented("memset: not implemented for queue type: {}", cl->queue->type);
    }
}

void memcpy_transfer(CommandListImpl *impl, kes_gpuptr_t dst, kes_gpuptr_t src, std::size_t size) {
    assert(impl->queue->type == KesQueueTypeTransfer, "memcpy_transfer: requires queue of Transfer type");

    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    // @todo: tmz?
    while (size > 0) {
        uint64_t bytes_written = enc.copy_linear(src, dst, size, false);
        size -= bytes_written;
        src += bytes_written;
        dst += bytes_written;
    }
}

void amdgpu_cmd_memcpy(KesCommandList pcl, kes_gpuptr_t dst, kes_gpuptr_t src, size_t size) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "memcpy: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case KesQueueTypeTransfer:
        memcpy_transfer(cl, dst, src, size);
        break;
    default:
        not_implemented("memcpy: not implemented for queue type: {}", cl->queue->type);
    }
}

SDMAAtomicOp sdma_atomic_op_map(KesSignal sig) {
    switch(sig) {
    case KesSignalAtomicSet:
        return SDMAAtomicOp::Swap;
    case KesSignalAtomicMax:
        return SDMAAtomicOp::UMax;
    case KesSignalAtomicOr:
        return SDMAAtomicOp::Or;
    default:
    not_implemented("sdma_atomic_op_map: no mapping for {}", sig);
    }
}

SDMAWaitMemOp sdma_waitmem_op_map(KesOp op) {
    switch(op) {
    case KesOpEqual:
        return SDMAWaitMemOp::Equal;
    default:
        not_implemented("sdma_waitmem_op_map: no mapping for {}", op);
    }
}

void wait_before_transfer(CommandListImpl *impl, kes_gpuptr_t ptr, uint64_t value, KesOp op, uint64_t mask) {
    assert(impl->queue->type == KesQueueTypeTransfer, "wait_before_transfer: requires queue of Transfer type");

    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    auto func = sdma_waitmem_op_map(op);

    // @todo: NOTE: this only writes the low 32-bits of value. I do not know how we should do this, it
    // seems the hardware doesn't support this.
    enc.wait_mem(func, ptr, value & 0xFFFFFFFF, mask & 0xFFFFFFFF);
}

void amdgpu_cmd_wait_before(KesCommandList pcl, KesStage after, kes_gpuptr_t ptr, uint64_t value, KesOp op, KesHazardFlags hazard, uint64_t mask) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "wait_before: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case KesQueueTypeTransfer:
        wait_before_transfer(cl, ptr, value, op, mask);
        break;
    default:
        not_implemented("wait_before: not implemented for queue type: {}", cl->queue->type);
    }
}

void signal_after_transfer(CommandListImpl *impl, kes_gpuptr_t ptr, uint64_t value, KesSignal sig) {
    assert(impl->queue->type == KesQueueTypeTransfer, "signal_after_transfer: requires queue of Transfer type");
    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    auto op = sdma_atomic_op_map(sig);

    enc.atomic(op, ptr, value);
}

void amdgpu_cmd_signal_after(KesCommandList pcl, KesStage before, kes_gpuptr_t ptr, uint64_t value, KesSignal sig) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "signal_after: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case KesQueueTypeTransfer:
        signal_after_transfer(cl, ptr, value, sig);
        break;
    default:
        not_implemented("wait_before: not implemented for queue type: {}", cl->queue->type);
    }
}

void write_timestamp_transfer(CommandListImpl *impl, kes_gpuptr_t ptr) {
    assert(impl->queue->type == KesQueueTypeTransfer, "write_timestamp_transfer: requires queue of Transfer type");
    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    enc.write_timestamp(ptr);
}

void amdgpu_cmd_write_timestamp(KesCommandList pcl, kes_gpuptr_t ptr) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "write_timestamp: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case KesQueueTypeTransfer:
        write_timestamp_transfer(cl, ptr);
        break;
    default:
        not_implemented("write_timestamp: not implemented for queue type: {}", cl->queue->type);
    }
}
