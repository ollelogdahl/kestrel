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

void analyze_uniformity(Compiler &);
void analyze_liveness(Compiler &);
void allocate_registers(Compiler &);
void codegen(Compiler &);

void rdna2_compile(gir::Module &mod, void *write_ptr, uint64_t base_addr) {
    Compiler compiler(mod);

    analyze_liveness(compiler);
    allocate_registers(compiler);
    analyze_uniformity(compiler);
    codegen(compiler);

    auto code = compiler.as.values();
    auto code_size_bytes = code.size() * sizeof(uint32_t);
    memcpy(write_ptr, code.data(), code_size_bytes);

    // dump the shader code to a file.
    // @todo: wip
    {
        std::stringstream ss;
        ss << "shader_" << std::hex << reinterpret_cast<uintptr_t>(write_ptr) << ".bin";
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

// @todo: stuff like this is pretty general.
void analyze_liveness(Compiler &cc) {
    for (uint32_t i = 0; i < cc.mod.insts.size(); ++i) {
        for (auto arg : cc.mod.insts[i].operands) {
            if (arg.id != 0xFFFFFFFF) cc.mod.insts[arg.id].meta.last_use = i;
        }
    }
}

void analyze_uniformity(Compiler &cc) {
    // Simple propagation: Root ptr is uniform.
    /*
    for (auto& inst : cc.mod.insts) {
        bool divergent = false;
        for (auto arg : inst.args) {
            if (arg.id != 0xFFFFFFFF && !cc.mod.values[arg.id].is_uniform) divergent = true;
        }
        if (inst.op == LOAD_GLOBAL) divergent = true; // Memory reads are divergent
        if (inst.dest.id != 0xFFFFFFFF) cc.mod.values[inst.dest.id].is_uniform = !divergent;
    }
    */
}

void allocate_registers(Compiler &cc) {
    // linear register allocation. We need to note the DS and determine how many
    // contiguous sgpr/vgprs are needed for that.
}

void codegen(Compiler &cc) {

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
