#include "compiler.h"
#include "rdna2_asm.h"

#include <sstream>
#include <iomanip>
#include <string>
#include <fstream>
#include <optional>

using namespace gir;

/*
*
* on rdna 2, 64-bit addresses are kept in a seq gpr-pair. This means that Refs to addr
* are actually 2 refs; should we do an 'expansion' pass?
*
* DCE
* memory access hoisting (load early). see ISA for details.
* LCIM
*
*
*/

// @todo: this is obviously very early stage wip...

struct Compiler {
    gir::Module& mod;
    RDNA2Assembler as;

    CompileShaderInfo shdr;

    uint32_t sgpr_allocator = 6;
    uint32_t vgpr_allocator = 3;
};

void lower_simple(Compiler &);
void lower_memory_loads(Compiler &);

void analyze_uniformity(Compiler &);
void codegen(Compiler &);

enum class AmdIntrinsics : uint32_t {
    GlobalLoadDwordAddTID_Scale4,
    GlobalStoreDwordAddTID_Scale4,
};

void tmp_dump_shader(uint32_t *data, size_t code_size_bytes) {
    std::stringstream ss;
    ss << "shader_tmp.bin";
    std::string filename = ss.str();

    std::ofstream outfile(filename, std::ios::out | std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(reinterpret_cast<const char*>(data), code_size_bytes);
        outfile.close();
    }
    log("shader written to {}", filename);
}

std::string_view rdna2_backend_intrinsic_to_string(uint32_t intrinsic_id) {
    switch(intrinsic_id) {
        case (uint32_t)AmdIntrinsics::GlobalLoadDwordAddTID_Scale4: return "GlobalLoadDwordAddTID_Scale4";
        case (uint32_t)AmdIntrinsics::GlobalStoreDwordAddTID_Scale4: return "GlobalStoreDwordAddTID_Scale4";
        default: return "?";
    }
}

void rdna2_compile(gir::Module &mod, CompileShaderInfo shdr, void *write_ptr, uint64_t base_addr) {
    Compiler compiler(mod);
    compiler.shdr = shdr;

    gir::pass_normalize(mod);
    lower_simple(compiler);
    lower_memory_loads(compiler);
    gir::pass_eliminate_dead_code(mod);
    analyze_uniformity(compiler);

    auto txt = gir::dump_module(mod, rdna2_backend_intrinsic_to_string);
    log("\nIntermediate Representation:\n{}", txt);

    codegen(compiler);

    auto code = compiler.as.values();
    auto code_size_bytes = code.size() * sizeof(uint32_t);
    memcpy(write_ptr, code.data(), code_size_bytes);

    tmp_dump_shader(code.data(), code_size_bytes);
}

void lower_simple(Compiler &cc) {
    for (uint32_t i = 0; i < cc.mod.insts.size(); ++i) {
        auto &inst = cc.mod.insts[i];
        if (inst.op == gir::Op::RootPtr) {
            // root pointer is passed as the user sgprs.
            // we don't actually have to do anything.
            inst.meta.phys_reg = 0;
            inst.meta.is_uniform = true;
        }

        if (inst.op == gir::Op::LocalInvocationIdX) {
            inst.meta.phys_reg = 0;
            inst.meta.is_uniform = false;
        }
        if (inst.op == gir::Op::LocalInvocationIdY) {
            inst.meta.phys_reg = 1;
            inst.meta.is_uniform = false;
        }
        if (inst.op == gir::Op::LocalInvocationIdZ) {
            inst.meta.phys_reg = 2;
            inst.meta.is_uniform = false;
        }

        if (inst.op == gir::Op::WorkgroupIdX) {
            inst.meta.phys_reg = cc.shdr.num_user_sgprs + 0;
            inst.meta.is_uniform = true;
        }
        if (inst.op == gir::Op::WorkgroupIdY) {
            inst.meta.phys_reg = cc.shdr.num_user_sgprs + 1;
            inst.meta.is_uniform = true;
        }
        if (inst.op == gir::Op::WorkgroupIdZ) {
            inst.meta.phys_reg = cc.shdr.num_user_sgprs + 2;
            inst.meta.is_uniform = true;
        }
    }
}

struct AddressPattern {
    Value base_ptr;
    bool is_tid_scaled_by_4 = false;
};

std::optional<AddressPattern> match_address_pattern(Compiler& cc, const Inst& addr) {
    AddressPattern pat;

    // Match: ptr
    if (addr.type == Type::Ptr && addr.op != Op::Add) {
        pat.base_ptr = addr.operands[0];
        return pat;
    }

    // Match: ptr + offset
    if (addr.op == Op::Add) {
        auto& lhs = cc.mod.deref(addr.operands[0]);
        auto& rhs = cc.mod.deref(addr.operands[1]);

        // After normalization, ptr should be on left
        if (lhs.type != Type::Ptr) {
            log("not normalized?");
            return std::nullopt; // Shouldn't happen after normalization
        }

        pat.base_ptr = addr.operands[0];

        // Check if offset is tid * 4
        if (rhs.op == Op::Mul) {
            auto& mul_lhs = cc.mod.deref(rhs.operands[0]);
            auto& mul_rhs = cc.mod.deref(rhs.operands[1]);

            if (mul_lhs.op == Op::LocalInvocationIndex &&
                mul_rhs.op == Op::Const &&
                mul_rhs.data.imm_i64 == 4) {
                pat.is_tid_scaled_by_4 = true;
                return pat;
            }
        }

        // Other offset patterns not yet supported
        log("other offset pattern?");
        return std::nullopt;
    }

    log("what? op: {}", (int)addr.op);
    return std::nullopt;
}

void lower_memory_loads(Compiler &cc) {
    // device memory loads should become global_load_dword or similar.
    // these kinds of instructions support a base ptr + imm offset or
    // base sgpr ptr + vgpr offset.

    // global_load_dword: saddr + voff (+ imm offset)
    // global_load_dword: vaddr (+ imm offset)
    // global_load_dword_addtid: saddr (+ imm offset) + 4 * local_invocation_id
    for (uint32_t i = 0; i < cc.mod.insts.size(); ++i) {
        auto &inst = cc.mod.insts[i];
        if (inst.op == Op::Load) {
            auto& addr = cc.mod.deref(inst.operands[0]);
            auto pat = match_address_pattern(cc, addr);
            if (!pat) {
                not_implemented("lower_memory_loads: unsupported load address pattern");
            }

            auto& base = cc.mod.deref(pat->base_ptr);

            if (!base.meta.is_uniform) {
                not_implemented("lower_memory_loads: non-uniform base pointer in Load not yet supported");
            }

            if (pat->is_tid_scaled_by_4) {
                inst.op = Op::BackendIntrinsic;
                inst.intrinsic_id = (uint32_t)AmdIntrinsics::GlobalLoadDwordAddTID_Scale4;
                inst.operands = {pat->base_ptr};
            } else {
                not_implemented("lower_memory_loads: simple loads not yet implemented");
            }
        } else if (inst.op == Op::Store) {
            auto& addr = cc.mod.deref(inst.operands[0]);
            auto pat = match_address_pattern(cc, addr);
            if (!pat) {
                not_implemented("lower_memory_loads: unsupported Store address pattern");
            }

            auto& base = cc.mod.deref(pat->base_ptr);

            if (!base.meta.is_uniform) {
                not_implemented("lower_memory_loads: non-uniform base pointer in Store not yet supported");
            }

            if (pat->is_tid_scaled_by_4) {
                inst.op = Op::BackendIntrinsic;
                inst.intrinsic_id = (uint32_t)AmdIntrinsics::GlobalStoreDwordAddTID_Scale4;
                inst.operands = {pat->base_ptr, inst.operands[1]};
            } else {
                not_implemented("lower_memory_loads: simple stores not yet implemented");
            }
        }
    }
}

void analyze_uniformity(Compiler &cc) {

}

uint32_t required_regs_for_type(gir::Type t) {
    switch(t) {
    case gir::Type::I32:
    case gir::Type::F32:
        return 1;
    case gir::Type::Ptr:
        return 2;
    }

    return 0;
}

void allocate_sgpr(Compiler &c, gir::Inst &inst) {
    if (inst.meta.phys_reg == ~0u) {
        inst.meta.phys_reg = c.sgpr_allocator++;
    }
}

void allocate_vgpr(Compiler &c, gir::Inst &inst) {
    if (inst.meta.phys_reg == ~0u) {
        inst.meta.phys_reg = c.sgpr_allocator++;
    }
}

inline RDNA2Assembler::vsrc get_vsrc(Compiler& c, gir::Value v) {
    auto& inst = c.mod.deref(v);

    // Handle inline constants
    if (inst.op == gir::Op::Const) {
        int32_t imm = inst.data.imm_i64;
        if (imm == 0) return RDNA2Assembler::vsrc::zero;
        if (imm == -1) return RDNA2Assembler::vsrc::int_neg_1;
        if (imm >= 1 && imm <= 64) return (RDNA2Assembler::vsrc)((uint)RDNA2Assembler::vsrc::int_pos_1 + imm - 1);
        if (imm < 0 && imm >= -16) return (RDNA2Assembler::vsrc)((uint)RDNA2Assembler::vsrc::int_neg_1 - imm - 1);
        // @todo: this needs to be additionally added after the
        return RDNA2Assembler::vsrc::literal_constant;
    }

    if (inst.meta.is_uniform) {
        allocate_sgpr(c, inst);
        return (RDNA2Assembler::vsrc)((uint)RDNA2Assembler::vsrc::sgpr0 + inst.meta.phys_reg);
    } else {
        allocate_vgpr(c, inst);
        return (RDNA2Assembler::vsrc)((uint)RDNA2Assembler::vsrc::vgpr0 + inst.meta.phys_reg);
    }
}

inline RDNA2Assembler::ssrc get_ssrc(Compiler& c, gir::Value v) {
    auto& inst = c.mod.deref(v);

    if (inst.op == gir::Op::Const) {
        int32_t imm = inst.data.imm_i64;
        if (imm == 0) return RDNA2Assembler::ssrc::src_zero;
        if (imm == -1) return RDNA2Assembler::ssrc::int_neg_1;
        if (imm >= 1 && imm <= 64) return (RDNA2Assembler::ssrc)((uint)RDNA2Assembler::ssrc::int_pos_1 + imm - 1);
        if (imm < 0 && imm >= -16) return (RDNA2Assembler::ssrc)((uint)RDNA2Assembler::ssrc::int_neg_1 - imm - 1);
        return RDNA2Assembler::ssrc::literal_constant;
    }

    assert(inst.meta.is_uniform, "Cannot use non-uniform value as ssrc");

    allocate_sgpr(c, inst);
    return (RDNA2Assembler::ssrc)((uint)RDNA2Assembler::ssrc::sgpr0 + inst.meta.phys_reg);
}

// some operators are commutative but don't allow certain values in a given place.
// Specifically, vsrc1 MUST be a VGPR, src0 can be anything (SGPR, VGPR, const).
struct VOP2Operands {
    RDNA2Assembler::vsrc src0;
    uint8_t src1;
};
VOP2Operands vop2_order(Compiler &cc, const Value &a, const Value &b) {
    auto& op0 = cc.mod.deref(a);
    auto& op1 = cc.mod.deref(b);

    bool op0_is_vgpr = !op0.meta.is_uniform && op0.op != gir::Op::Const;
    bool op1_is_vgpr = !op1.meta.is_uniform && op1.op != gir::Op::Const;

    if (!op0_is_vgpr && !op1_is_vgpr) {
        not_implemented("codegen: vop2_order requires at least one VGPR operand");
    }

    if (op0_is_vgpr) {
        return VOP2Operands{get_vsrc(cc, b), (uint8_t)op0.meta.phys_reg};
    } else {
        return VOP2Operands{get_vsrc(cc, a), (uint8_t)op1.meta.phys_reg};
    }
}

void codegen(Compiler &cc) {
    for (auto &inst : cc.mod.insts) {
        switch (inst.op) {
        case gir::Op::BackendIntrinsic: {
            switch(inst.intrinsic_id) {
            case (uint32_t)AmdIntrinsics::GlobalLoadDwordAddTID_Scale4: {
                auto saddr = get_ssrc(cc, inst.operands[0]);

                allocate_vgpr(cc, inst);

                // @todo: how do we know what to do about the cache flags?
                cc.as.global(
                    RDNA2Assembler::global_opcode::global_load_dword_addtid,
                    false, false, false, false,
                    0,
                    inst.meta.phys_reg,
                    (uint8_t)saddr,
                    0,
                    0
                );

                // @todo: wait for load to complete. this is very conservative
                cc.as.sopp(RDNA2Assembler::sopp_opcode::s_waitcnt, 0x3F70);
            } break;
            case (uint32_t)AmdIntrinsics::GlobalStoreDwordAddTID_Scale4: {
                auto saddr = get_ssrc(cc, inst.operands[0]);
                auto& data = cc.mod.deref(inst.operands[1]);

                if (data.meta.is_uniform) {
                    not_implemented("codegen: GlobalStoreDwordAddTI with uniform data (need v_mov)");
                }

                cc.as.global(
                    RDNA2Assembler::global_opcode::global_store_dword_addtid,
                    false, false, false, false,
                    0, 0, (uint8_t)saddr, data.meta.phys_reg, 0
                );
            } break;
            default:
                not_implemented("codegen: unknown backend intrinsic: {}", inst.intrinsic_id);
            }
        } break;

        case gir::Op::Store: {
            // @todo: we currently assume all stores are global, but this may not be the case.
            // I am not sure how NIR handles this, nor how other backends have local caches (LDS & GDS).
            auto& addr = cc.mod.deref(inst.operands[0]);
            auto& data = cc.mod.deref(inst.operands[1]);

            if (!addr.meta.is_uniform) {
                not_implemented("codegen: Store with non-uniform address not yet supported");
            }

            if (data.meta.is_uniform) {
                not_implemented("codegen: Store with uniform data not yet supported (need v_mov to copy sgpr to vgpr)");
            }

            if (data.type != gir::Type::I32 && data.type != gir::Type::F32) {
                not_implemented("codegen: Store only supports I32/F32 for now");
            }

            // global_store_dword: saddr + vdata
            auto saddr = get_ssrc(cc, inst.operands[0]);

            cc.as.global(
                RDNA2Assembler::global_opcode::global_store_dword,
                false, false, false, false,
                0,                      // 12-bit immediate offset
                0,                      // vdst (unused for stores)
                (uint8_t)saddr,         // saddr base pointer
                data.meta.phys_reg,     // vdata - data to store
                0                       // vaddr (0 = use saddr only)
            );
        } break;

        case gir::Op::Add: {
            if (inst.type == gir::Type::I32) {
                if (inst.meta.is_uniform) {
                    allocate_sgpr(cc, inst);
                    auto src0 = get_ssrc(cc, inst.operands[0]);
                    auto src1 = get_ssrc(cc, inst.operands[1]);
                    cc.as.sop2(
                        RDNA2Assembler::sop2_opcode::s_add_u32,
                        (RDNA2Assembler::ssrc)inst.meta.phys_reg,
                        src0,
                        src1
                    );
                } else {
                    allocate_vgpr(cc, inst);
                    auto ops = vop2_order(cc, inst.operands[0], inst.operands[1]);

                    cc.as.vop2(
                        RDNA2Assembler::vop2_opcode::v_add_nc_u32,
                        inst.meta.phys_reg,
                        ops.src0,
                        ops.src1
                    );
                }
            } else if (inst.type == gir::Type::Ptr) {
                not_implemented("codegen: pointer addition (64-bit) not yet implemented");
            } else {
                not_implemented("codegen: Add not implemented for type: {}", (int)inst.type);
            }
        } break;
        case gir::Op::WorkgroupBarrier: {
            cc.as.sopp(RDNA2Assembler::sopp_opcode::s_barrier, 0);
        } break;
        case gir::Op::SubgroupBarrierInit: {
            // this initializes with the first active thread value in a vgpr. If we
            // have a scalar value first, we must move it.
            //
            // @todo: this MUST be an actual register, not a vsrc.
            auto data = (uint8_t)((int)get_vsrc(cc, inst.operands[0]) - 256);
            auto offset0 = inst.data.barrier_data.resource_id;
            cc.as.ds(RDNA2Assembler::ds_opcode::ds_gws_init, true, offset0, 0, 0, data, 0, 0);
        } break;
        case gir::Op::SubgroupBarrierSignal: {
            auto offset0 = inst.data.barrier_data.resource_id;
            cc.as.ds(RDNA2Assembler::ds_opcode::ds_gws_sema_v, true, offset0, 0, 0, 0, 0, 0);
        } break;
        case gir::Op::SubgroupBarrierWait: {
            auto offset0 = inst.data.barrier_data.resource_id;
            cc.as.ds(RDNA2Assembler::ds_opcode::ds_gws_sema_p, true, offset0, 0, 0, 0, 0, 0);
        } break;

        case gir::Op::Const:
        case gir::Op::RootPtr:
        case gir::Op::LocalInvocationIdX:
        case gir::Op::LocalInvocationIdY:
        case gir::Op::LocalInvocationIdZ:
        case gir::Op::WorkgroupIdX:
        case gir::Op::WorkgroupIdY:
        case gir::Op::WorkgroupIdZ:
            // Skip metadata operations and constants
            break;
        default:
            not_implemented("codegen: operation not yet implemented: {}", (int)inst.op);
            break;
        }
    }

    cc.as.sopp(RDNA2Assembler::sopp_opcode::s_endpgm, 0);

    // (RDNA ISA Ref. 2.5)
    for (auto i = 0; i < 64; ++i) {
        cc.as.sopp(RDNA2Assembler::sopp_opcode::s_code_end, 0);
    }
}
