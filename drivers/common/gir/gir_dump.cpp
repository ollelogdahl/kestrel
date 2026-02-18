#include "kestrel/gir.h"
#include <string>
#include <sstream>

namespace gir {

static const std::string type_str[] = {
    "Void",
    "I32",
    "F32",
    "Ptr",
};

static const std::string op_str[] = {
    "Add",
    "Sub",
    "Mul",
    "Div",
    "Mod",
    "FAdd",
    "FSub",
    "FMul",
    "FDiv",
    "And",
    "Or",
    "Xor",
    "Shl",
    "Shr",
    "Eq",
    "Ne",
    "Lt",
    "Le",
    "Gt",
    "Ge",
    "Load",
    "LoadShared",
    "Store",
    "StoreShared",
    "Const",
    "RootPtr",
    "LocalInvocationIdX",
    "LocalInvocationIdY",
    "LocalInvocationIdZ",
    "LocalInvocationIndex",
    "WorkgroupIdX",
    "WorkgroupIdY",
    "WorkgroupIdZ",
    "SubgroupId",
    "SubgroupSize",
    "NumSubgroups",
    "GlobalInvocationIdX",
    "GlobalInvocationIdY",
    "GlobalInvocationIdZ",
    "GlobalInvocationIndex",
    "WorkgroupBarrier",
    "SubgroupBarrierInit",
    "SubgroupBarrierWait",
    "SubgroupBarrierSignal",
    "BackendIntrinsic",
};

std::string dump_module(Module &mod, std::function<std::string_view(uint32_t)> backend_intrinsic_to_string) {
    std::stringstream ss;

    ss << "; gir " << GIR_VERSION << std::endl;
    ss << "; instructions: " << mod.insts.size() << std::endl;

    for (uint32_t i = 0; i < mod.insts.size(); ++i) {
        auto &inst = mod.insts[i];
        auto op_text = op_str[(uint32_t)inst.op];
        if (inst.op == Op::BackendIntrinsic) {
            op_text = backend_intrinsic_to_string(inst.intrinsic_id);
        }

        ss << "$" << i << " = " << op_text;

        for (auto j = 0; j < inst.operands.size(); ++j) {
            auto &operand = inst.operands[j];
            if (j != 0) {
                ss << ",";
            }
            ss << " $" << operand.id;
        }

        switch (inst.op) {
        case Op::SubgroupBarrierWait:
        case Op::SubgroupBarrierSignal:
            ss << " resource_id=" << inst.data.barrier_data.resource_id;
            break;
        default:
            break;
        }

        ss << std::endl;
    }

    return ss.str();
}

};
