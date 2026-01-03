#include "gpuinfo.h"
#include "kestrel/kestrel.h"
#include "impl.h"

#include "amdgfxregs.h"
#include "sid.h"
#include <amdgpu_drm.h>

#include "pm4_encoder.h"
#include "sdma_encoder.h"
#include <cstdint>

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

struct Shader {};

struct DispatchInfo {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint64_t indirect_va;
};

void amdgpu_emit_dispatch_packets(GpuInfo &ginfo, Pm4Encoder &enc, Shader &shader, DispatchInfo &dinfo) {

    // @todo: get this from device settings
    uint32_t dispatch_initiator = S_00B800_COMPUTE_SHADER_EN(1) | S_00B800_TUNNEL_ENABLE(1);

    // @todo: support predicating
    auto predicating = false;

    // @todo: support
    auto ordered = false;
    if (ordered) {
        dispatch_initiator &= ~S_00B800_ORDER_MODE(1);
    }

    // @todo: get from shader info
    auto wave_size = 32;
    if (wave_size == 32) {
        dispatch_initiator |= S_00B800_CS_W32_EN(1);
    }

    if (dinfo.indirect_va) {
        // mesa align32 workaround not needed; only for GFX7
        enc.emit(PKT3(PKT3_DISPATCH_INDIRECT, 2, 0) | PKT3_SHADER_TYPE_S(1));
        enc.emit(dinfo.indirect_va);
        enc.emit(dinfo.indirect_va >> 32);
        enc.emit(dispatch_initiator);
    } else {
        // @todo: load block-size into shader regs.
        // @todo: unaligned?
        enc.emit(PKT3(PKT3_DISPATCH_DIRECT, 3, predicating) | PKT3_SHADER_TYPE_S(1));
        enc.emit(dinfo.x);
        enc.emit(dinfo.y);
        enc.emit(dinfo.z);
        enc.emit(dispatch_initiator);
    }
}

void amdgpu_cmd_dispatch(KesCommandList pcl, uint32_t x, uint32_t y, uint32_t z) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "dispatch: command list handle invalid: {}", (void *)pcl);

    auto hw_ip_type = hw_ip_type_from_queue_type(cl->queue->type);
    Pm4Encoder enc(cl->queue->dev->info, hw_ip_type, cl->cs);

    Shader tmp{};
    DispatchInfo dinfo{
        .x = x,
        .y = y,
        .z = z,
        .indirect_va = 0
    };

    amdgpu_emit_dispatch_packets(cl->queue->dev->info, enc, tmp, dinfo);
}

void amdgpu_cmd_dispatch_indirect(KesCommandList pcl, uint64_t indirect_addr) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "dispatch: command list handle invalid: {}", (void *)pcl);

    auto hw_ip_type = hw_ip_type_from_queue_type(cl->queue->type);
    Pm4Encoder enc(cl->queue->dev->info, hw_ip_type, cl->cs);

    Shader tmp{};
    DispatchInfo dinfo{
        .indirect_va = indirect_addr
    };

    amdgpu_emit_dispatch_packets(cl->queue->dev->info, enc, tmp, dinfo);
}
