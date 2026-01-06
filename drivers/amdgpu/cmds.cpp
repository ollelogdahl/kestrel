#include "cp_encoder.h"
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

AtomicOp atomic_op_map(KesSignal sig) {
    switch(sig) {
    case KesSignalAtomicSet:
        return AtomicOp::Swap;
    case KesSignalAtomicMax:
        return AtomicOp::UMax;
    case KesSignalAtomicOr:
        return AtomicOp::Or;
    default:
    not_implemented("sdma_atomic_op_map: no mapping for {}", sig);
    }
}

WaitMemOp waitmem_op_map(KesOp op) {
    switch(op) {
    case KesOpEqual:
        return WaitMemOp::Equal;
    default:
        not_implemented("sdma_waitmem_op_map: no mapping for {}", op);
    }
}

void wait_before_transfer(CommandListImpl *impl, kes_gpuptr_t ptr, uint64_t value, KesOp op, uint64_t mask) {
    assert(impl->queue->type == KesQueueTypeTransfer, "wait_before_transfer: requires queue of Transfer type");
    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    auto func = waitmem_op_map(op);

    // @todo: NOTE: this only reads the low 32-bits of value. I do not know how we should do this, it
    // seems the hardware doesn't support this.
    enc.wait_mem(func, ptr, value & 0xFFFFFFFF, mask & 0xFFFFFFFF);
}

void wait_before_gcs(CommandListImpl *impl, kes_gpuptr_t ptr, uint64_t value, KesOp op, uint64_t mask) {
    assert(impl->queue->type == KesQueueTypeGraphics || impl->queue->type == KesQueueTypeCompute,
        "wait_before_gcs: requires Graphics or Compute queue");
    CPEncoder enc(impl->queue->dev->info, impl->queue->hw_ip_type, impl->cs);

    auto func = waitmem_op_map(op);

    // @todo: NOTE: this only reads the low 32-bits of value. I do not know how we should do this, it
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
    case KesQueueTypeCompute:
    case KesQueueTypeGraphics:
        wait_before_gcs(cl, ptr, value, op, mask);
        break;
    default:
        not_implemented("wait_before: not implemented for queue type: {}", cl->queue->type);
    }
}

void signal_after_transfer(CommandListImpl *impl, KesStage before, kes_gpuptr_t ptr, uint64_t value, KesSignal sig) {
    assert(impl->queue->type == KesQueueTypeTransfer, "signal_after_transfer: requires queue of Transfer type");
    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    // @todo: how do we await before? I guess in some ways, it doesn't make sense.
    // MESA does a nop, as a nop ensures that all prev sdma transfers are finished.

    auto op = atomic_op_map(sig);

    enc.atomic(op, ptr, value);
}

void signal_after_gcs(CommandListImpl *impl, KesStage before, kes_gpuptr_t ptr, uint64_t value, KesSignal sig) {
    assert(impl->queue->type == KesQueueTypeGraphics || impl->queue->type == KesQueueTypeCompute,
        "signal_after_gcs: requires Graphics or Compute queue");

    CPEncoder enc(impl->queue->dev->info, impl->queue->hw_ip_type, impl->cs);

    // @todo: ensure this is correct and stuff.
    uint32_t event_type;
    if (before == KesStagePixelShader) {
        event_type = V_028A90_PS_DONE;
        assert(impl->queue->type == KesQueueTypeGraphics, "signal_after_gcs: PS_DONE only valid on graphics queue");
    } else if (before == KesStageCompute) {
        event_type = V_028A90_CS_DONE;
    } else {
        event_type = V_028A90_BOTTOM_OF_PIPE_TS;
    }

    // signaling with atomic set is more efficient and handled as an edge case
    // release mem can actually set a value atomically.
    if (sig == KesSignalAtomicSet) {
        enc.release_mem(
            event_type,
            0,
            EOP_DST_SEL_MEM,
            EOP_INT_SEL_SEND_DATA_AFTER_WR_CONFIRM,
            EOP_DATA_SEL_VALUE_32BIT,
            ptr,
            (uint32_t)value
        );
    } else {
        // first wait for the barrier event, then perform atomic op.
        enc.release_mem(
            event_type,
            0,
            EOP_DST_SEL_MEM,
            EOP_INT_SEL_NONE,
            EOP_DATA_SEL_DISCARD,
            0,
            0
        );
        auto op = atomic_op_map(sig);
        enc.atomic_mem(
            op,
            ATOMIC_COMMAND_LOOP,
            ptr,
            value,
            0  // compare_data (unused for most ops)
        );
    }
}

void amdgpu_cmd_signal_after(KesCommandList pcl, KesStage before, kes_gpuptr_t ptr, uint64_t value, KesSignal sig) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "signal_after: command list handle invalid: {}", (void *)pcl);

    // @todo: the signaling (atomic write) on transfer queue skips over caching. On Compute/Gfx, it works differently.
    // we need to ensure that we emit the proper cache flushing before writing the value.

    switch(cl->queue->type) {
    case KesQueueTypeTransfer:
        signal_after_transfer(cl, before, ptr, value, sig);
        break;
    case KesQueueTypeCompute:
    case KesQueueTypeGraphics:
        signal_after_gcs(cl, before, ptr, value, sig);
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

void write_timestamp_gcs(CommandListImpl *impl, kes_gpuptr_t ptr) {
    assert(impl->queue->type == KesQueueTypeGraphics || impl->queue->type == KesQueueTypeCompute,
        "write_timestamp_gcs: requires Graphics or Compute queue");
    CPEncoder enc(impl->queue->dev->info, impl->queue->hw_ip_type, impl->cs);

    // @todo: handle TopOfPipe in a special way (see mesa radv_write_timestamp).
    enc.release_mem(
        V_028A90_BOTTOM_OF_PIPE_TS,
        0,
        EOP_DST_SEL_MEM,
        EOP_INT_SEL_SEND_DATA_AFTER_WR_CONFIRM,
        EOP_DATA_SEL_TIMESTAMP, ptr,
        0);
}

void amdgpu_cmd_write_timestamp(KesCommandList pcl, kes_gpuptr_t ptr) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "write_timestamp: command list handle invalid: {}", (void *)pcl);

    switch(cl->queue->type) {
    case KesQueueTypeTransfer:
        write_timestamp_transfer(cl, ptr);
        break;
    case KesQueueTypeCompute:
    case KesQueueTypeGraphics:
        write_timestamp_gcs(cl, ptr);
        break;
    default:
        not_implemented("write_timestamp: not implemented for queue type: {}", cl->queue->type);
    }
}

struct DispatchInfo {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint64_t indirect_va;
    uint64_t data_va;
};

struct ShaderRegs {
    uint32_t pgm_lo;
    uint32_t pgm_hi;
    uint32_t pgm_rsrc1;
    uint32_t pgm_rsrc2;
    uint32_t pgm_rsrc3;

    uint32_t userdata_0;
};

enum class HwStage {
    Compute
};

struct ShaderInfo {
    uint32_t block_size[3];
    HwStage hw_stage;
    ShaderRegs regs;

    uint32_t wave_size;
};

struct ShaderConfig {
    uint32_t pgm_rsrc1;
    uint32_t pgm_rsrc2;
    uint32_t pgm_rsrc3;
    uint32_t compute_resource_limits;

    uint32_t user_sgpr_count;
};

struct Shader {
    ShaderInfo info;
    ShaderConfig config;
    uint64_t va;
};

void init_compute_shader_config(DeviceImpl *dev, Shader &shader) {

    // @todo: ultra temporary.
    auto x = amdgpu_malloc(dev, 1024, 256, KesMemoryDefault);
    uint32_t *pgm = (uint32_t *)x.cpu;
    auto pgmidx = 0;
    // pgm[pgmidx++] = 0x24020402; // v_lshlrev_b32 v1, 2, v0
    // pgm[pgmidx++] = 0x4A020300; // v_add_co_u32 v1, vcc_lo, s0, v1
    // pgm[pgmidx++] = 0x4A040201; // v_add_co_ci_u32 v2, vcc_lo, s1, 0, vcc_lo
    // pgm[pgmidx++] = 0xDC500000; // global_store_dword v[1:2], v0, off
    // pgm[pgmidx++] = 0x00000001;
    pgm[pgmidx++] = 0xBF810000; // s_endpgm

    // (RDNA ISA Ref. 2.5)
    for (auto i = 0; i < 64; ++i) {
        pgm[pgmidx++] = 0xBF9F0000; // s_code_end
    }

    log("shader code: {} {}", (void *)x.cpu, (void *)x.gpu);

    // @todo: temporary
    auto wave_size = 32;
    auto waves_per_threadgroup = 1;
    auto max_waves_per_sh = 0x3FF;
    auto threadgroups_per_cu = 1;

    // Fixed for the Root Pointer ABI
    auto num_user_sgprs = 2;

    auto num_vgprs = 8;
    auto num_sgprs = 8;
    auto num_shared_vgprs = 1;
    auto scratch_enabled = false;
    auto trap_present = false;

    auto dx10_clamp = true;

    auto num_shared_vgpr_blocks = num_shared_vgprs / 8;

    shader.config.user_sgpr_count = num_user_sgprs;
    shader.info.wave_size = wave_size;
    shader.info.block_size[0] = 32;
    shader.info.block_size[1] = 1;
    shader.info.block_size[2] = 1;
    shader.va = x.gpu;
    shader.info.hw_stage = HwStage::Compute;

    // use large limits.
    shader.config.compute_resource_limits =
          S_00B854_SIMD_DEST_CNTL(waves_per_threadgroup % 4 == 0)
        | S_00B854_WAVES_PER_SH(max_waves_per_sh)
        | S_00B854_CU_GROUP_COUNT(threadgroups_per_cu - 1);

    shader.config.pgm_rsrc1 =
          S_00B848_VGPRS((num_vgprs - 1) / (wave_size == 32 ? 8 : 4))
        | S_00B848_DX10_CLAMP(dx10_clamp)
        | S_00B128_MEM_ORDERED(true); //always true for gfx10.3

    shader.config.pgm_rsrc2 =
          S_00B84C_USER_SGPR(shader.config.user_sgpr_count)
        | S_00B22C_USER_SGPR_MSB_GFX10(num_user_sgprs >> 5)
        | S_00B12C_SCRATCH_EN(scratch_enabled)
        | S_00B12C_TRAP_PRESENT(trap_present)
        | S_00B84C_TGID_X_EN(1)
        | S_00B84C_TGID_Y_EN(1)
        | S_00B84C_TGID_Z_EN(1);

    shader.config.pgm_rsrc3 =
          S_00B8A0_SHARED_VGPR_CNT(num_shared_vgpr_blocks);
}

void precompute_regs(ShaderInfo &info) {
    auto &regs = info.regs;

    // @todo: setup that compute_resource_limits thingy.

    switch(info.hw_stage) {
    case HwStage::Compute:
        regs.pgm_lo = R_00B830_COMPUTE_PGM_LO;
        regs.pgm_hi = R_00B834_COMPUTE_PGM_HI;
        regs.pgm_rsrc1 = R_00B848_COMPUTE_PGM_RSRC1;
        regs.pgm_rsrc2 = R_00B84C_COMPUTE_PGM_RSRC2;
        regs.pgm_rsrc3 = R_00B8A0_COMPUTE_PGM_RSRC3;
        regs.userdata_0 = R_00B900_COMPUTE_USER_DATA_0;
        break;
    }
}

void emit_compute_shader(Shader &shader, Pm4Encoder &enc) {
    enc.set_sh_reg(shader.info.regs.pgm_lo, shader.va >> 8);
    enc.set_sh_reg(shader.info.regs.pgm_hi, shader.va >> 40);

    enc.set_sh_reg(shader.info.regs.pgm_rsrc1, shader.config.pgm_rsrc1);
    enc.set_sh_reg(shader.info.regs.pgm_rsrc2, shader.config.pgm_rsrc2);
    enc.set_sh_reg(shader.info.regs.pgm_rsrc3, shader.config.pgm_rsrc3);

    enc.set_sh_reg(R_00B854_COMPUTE_RESOURCE_LIMITS, shader.config.compute_resource_limits);

    enc.set_sh_reg_seq(R_00B81C_COMPUTE_NUM_THREAD_X, 3);
    enc.emit(shader.info.block_size[0] & 0xFFFF);
    enc.emit(shader.info.block_size[1] & 0xFFFF);
    enc.emit(shader.info.block_size[2] & 0xFFFF);
}

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

    if (shader.info.wave_size == 32) {
        dispatch_initiator |= S_00B800_CS_W32_EN(1);
    }

    emit_compute_shader(shader, enc);

    uint32_t regs[2];
    regs[0] = dinfo.data_va;
    regs[1] = dinfo.data_va >> 32;

    // emit user data pointers.
    enc.set_sh_reg_seq(shader.info.regs.userdata_0, shader.config.user_sgpr_count);
    for (auto i = 0; i < shader.config.user_sgpr_count; ++i) {
        enc.emit(regs[i]);
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

void amdgpu_cmd_dispatch(KesCommandList pcl, kes_gpuptr_t data, uint32_t x, uint32_t y, uint32_t z) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "dispatch: command list handle invalid: {}", (void *)pcl);

    auto hw_ip_type = hw_ip_type_from_queue_type(cl->queue->type);
    Pm4Encoder enc(cl->queue->dev->info, hw_ip_type, cl->cs);

    Shader tmp{};
    DispatchInfo dinfo{
        .x = x,
        .y = y,
        .z = z,
        .indirect_va = 0,
        .data_va = data,
    };

    // @todo: do this earlier.
    init_compute_shader_config(cl->queue->dev, tmp);
    precompute_regs(tmp.info);

    amdgpu_emit_dispatch_packets(cl->queue->dev->info, enc, tmp, dinfo);
}

void amdgpu_cmd_dispatch_indirect(KesCommandList pcl, kes_gpuptr_t data, kes_gpuptr_t indirect_addr) {
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl, "dispatch: command list handle invalid: {}", (void *)pcl);

    auto hw_ip_type = hw_ip_type_from_queue_type(cl->queue->type);
    Pm4Encoder enc(cl->queue->dev->info, hw_ip_type, cl->cs);

    Shader tmp{};
    DispatchInfo dinfo{
        .indirect_va = indirect_addr,
        .data_va = data,
    };

    amdgpu_emit_dispatch_packets(cl->queue->dev->info, enc, tmp, dinfo);
}
