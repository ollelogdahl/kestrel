#include "compiler.h"
#include "rdna2_asm.h"
#include "gir/gir.h"

#include <sstream>
#include <iomanip>
#include <string>
#include <fstream>

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
};

void lower_simple(Compiler &);
void analyze_uniformity(Compiler &);
void allocate_registers(Compiler &);
void codegen(Compiler &);

enum class AmdIntrinsics : uint32_t {
    GlobalLoadDword,
    GlobalLoadDwordAddTI, // 12-bit imm offset, saddr, addr
};

void rdna2_compile(gir::Module &mod, void *write_ptr, uint64_t base_addr) {
    Compiler compiler(mod);

    lower_simple(compiler);
    analyze_uniformity(compiler);
    allocate_registers(compiler);
    codegen(compiler);

    auto code = compiler.as.values();
    auto code_size_bytes = code.size() * sizeof(uint32_t);
    memcpy(write_ptr, code.data(), code_size_bytes);

    // dump the shader code to a file.
    // @todo: wip
    {
        std::stringstream ss;
        ss << "shader_tmp.bin";
        std::string filename = ss.str();

        std::ofstream outfile(filename, std::ios::out | std::ios::binary);
        if (outfile.is_open()) {
            outfile.write(reinterpret_cast<const char*>(code.data()), code_size_bytes);
            outfile.close();
        }
        log("shader written to {}", filename);
        exit(0);
    }
}

void lower_simple(Compiler &cc) {
    for (uint32_t i = 0; i < cc.mod.insts.size(); ++i) {
        auto &inst = cc.mod.insts[i];
        if (inst.op == gir::Op::GetRootPtr) {
            // root pointer is passed as the user sgprs.
            // we don't actually have to do anything.
            inst.meta.phys_reg = 0;
            inst.meta.is_uniform = true;
        }

        // @todo: handle local_invocation_id.
        // There are many ways to do this, but I believe we need to lower it
        // into a pack operation of vgpr0,1,2. But I'm not entirely sure.
    }
}

void lower_memory_loads(Compiler &cc) {
    // device memory loads should become global_load_dword or similar.
    // these kinds of instructions support a base ptr + imm offset or
    // base sgpr ptr + vgpr offset.

    // if we detect such a pattern we can replace with these opcodes.
    // global_load_dword: saddr + voff (+ imm offset)
    // global_load_dword: vaddr (+ imm offset)
    // global_load_dword_addtid: saddr (+ imm offset) + 4 * local_invocation_id
    for (uint32_t i = 0; i < cc.mod.insts.size(); ++i) {
        auto &inst = cc.mod.insts[i];
        if (inst.op == gir::Op::Load) {
            auto addr = cc.mod.deref(inst.operands[0]);

            if (addr.meta.is_uniform) {
                not_implemented("lower_memory_loads: cannot handle Op::Load with uniform address");
            }

            if (addr.op == gir::Op::Add) {
                // we have detected an offset!
                // @todo: I think we need some form of canonicalization
                // here so the check can be more trivial.

                auto lhs = cc.mod.deref(addr.operands[0]);
                auto rhs = cc.mod.deref(addr.operands[1]);

                assert(lhs.type == gir::Type::Ptr, "lower_memory_loads: invalid operand in load(x + y)");

                if (rhs.op == gir::Op::Mul) {
                    auto lhs2 = cc.mod.deref(rhs.operands[0]);
                    auto rhs2 = cc.mod.deref(rhs.operands[1]);

                    if (lhs2.op == gir::Op::GetLocalInvocationId && rhs2.op == gir::Op::Const && rhs2.data.imm_i64 == 4) {
                        // replace instruction
                        auto args = std::vector<Value>{lhs, rhs2};
                        inst = gir::Inst{
                            .op = gir::Op::BackendIntrinsic,
                            .type = gir::Type::I32,
                            .operands = args,
                            .intrinsic_id = AmdIntrinsics::GlobalLoadDwordAddTI
                        }
                    }
                }

            } else {

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

void allocate_registers(Compiler &cc) {
    uint32_t sgpr_start = 6;
    uint32_t vgpr_start = 3;
    // @todo: improve register allocation and also reuse non-live
    // registers.
    for (auto &inst : cc.mod.insts) {
        if (inst.meta.phys_reg != ~0u) continue;

        // @todo: additionally, some types require a kind of align/size difference
        // (flat_load_dword16 needs 4 align, 16 size).
        // find next one / seq of regs of this kind
        auto count = required_regs_for_type(inst.type);

        if (inst.meta.is_uniform) {
            inst.meta.phys_reg = sgpr_start;
            sgpr_start += count;
        } else {
            inst.meta.phys_reg = vgpr_start;
            vgpr_start += count;
        }
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
        return RDNA2Assembler::vsrc::literal_constant;
    }

    auto reg = inst.meta.phys_reg;
    if (inst.meta.is_uniform) {
        return (RDNA2Assembler::vsrc)((uint)RDNA2Assembler::vsrc::sgpr0 + reg);
    } else {
        return (RDNA2Assembler::vsrc)((uint)RDNA2Assembler::vsrc::vgpr0 + reg);
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
    return (RDNA2Assembler::ssrc)((uint)RDNA2Assembler::ssrc::sgpr0 + inst.meta.phys_reg);
}

void codegen(Compiler &cc) {

    for (auto &inst : cc.mod.insts) {
        switch (inst.op) {
        case gir::Op::BackendIntrinsic: {
            switch(inst.intrinsic_id) {
            case (uint32_t)AmdIntrinsics::GlobalLoadDwordAddTI: {
                // @todo: support offset constants
                //assert(cc.mod.deref(inst.operands[0]).op == gir::Op::Const, "offset must be const");
                //auto offset = mod.deref(inst.operands[0]).data.imm_i64;
                auto offset = 0;

                auto saddr = get_ssrc(cc, inst.operands[1]);
                auto addr = get_vsrc(cc, inst.operands[2]);
                cc.as.global(RDNA2Assembler::global_opcode::global_load_dword_addtid, false, false, false, false,
                    offset, 0, (uint8_t)saddr, inst.meta.phys_reg, (uint8_t)addr
                );
            } break;
            }
        } break;
        }
    }

    /*
    for (auto& inst : mod.insts) {
        switch (inst.op) {
            case ADD:
                if (mod.values[inst.dest.id].is_uniform)
                    as.sop2(sop2_opcode::s_add_u32, mod.values[inst.dest.id].phys_reg,
                            mod.values[inst.args[0].id].phys_reg, mod.values[inst.args[1].id].phys_reg);
                else
                    as.vop2(vop2_opcode::v_add_nc_u32, mod.values[inst.dest.id].phys_reg,
                            mod.values[inst.args[0].id].phys_reg, mod.values[inst.args[1].id].phys_reg);
                break;
            case LOAD_GLOBAL:
                as.global(global_opcode::global_load_dword, inst.imm,
                            mod.values[inst.dest.id].phys_reg, mod.values[inst.args[0].id].phys_reg, 0);
                break;
            case STORE_GLOBAL:
                as.global(global_opcode::global_store_dword, inst.imm,
                            0, mod.values[inst.args[0].id].phys_reg, mod.values[inst.args[2].id].phys_reg);
                break;
            case V_MOV_S2V:
                    as.vop2(vop2_opcode::v_mov_b32, mod.values[inst.dest.id].phys_reg,
                            mod.values[inst.args[0].id].phys_reg, 0);
                    break;
        }
    }
     */


    cc.as.sopp(RDNA2Assembler::sopp_opcode::s_endpgm, 0);

    // (RDNA ISA Ref. 2.5)
    for (auto i = 0; i < 64; ++i) {
        cc.as.sopp(RDNA2Assembler::sopp_opcode::s_code_end, 0);
    }
}
