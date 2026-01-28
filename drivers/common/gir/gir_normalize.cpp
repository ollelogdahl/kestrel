#include "gir.h"

namespace gir {

// Canonicalize address computations so pointer is always on the left side of Add
void normalize_address_computation(Module& mod) {
    for (auto& inst : mod.insts) {
        if (inst.op != Op::Add || inst.type != Type::Ptr) continue;

        auto& lhs = mod.deref(inst.operands[0]);
        auto& rhs = mod.deref(inst.operands[1]);

        // Swap if pointer is on the right
        if (lhs.type != Type::Ptr && rhs.type == Type::Ptr) {
            std::swap(inst.operands[0], inst.operands[1]);
        }
    }
}

void pass_normalize(Module &mod) {
    normalize_address_computation(mod);
}

void pass_eliminate_dead_code(Module& mod) {
    std::vector<bool> is_live(mod.insts.size(), false);

    // Mark all instructions with side effects as live (roots)
    for (size_t i = 0; i < mod.insts.size(); ++i) {
        auto& inst = mod.insts[i];
        // Instructions with side effects are roots
        if (inst.op == Op::Store || inst.op == Op::BackendIntrinsic) {
            is_live[i] = true;
        }
    }

    // Propagate liveness backwards through dependencies
    // Keep iterating until no new instructions are marked live
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < mod.insts.size(); ++i) {
            if (!is_live[i]) continue;

            // Mark all operands of live instructions as live
            for (auto& op : mod.insts[i].operands) {
                if (op.is_inst() && !is_live[op.id]) {
                    is_live[op.id] = true;
                    changed = true;
                }
            }
        }
    }

    // Build new instruction list with value remapping
    std::vector<Inst> new_insts;
    std::vector<uint32_t> value_map(mod.insts.size());

    for (size_t i = 0; i < mod.insts.size(); ++i) {
        if (is_live[i]) {
            auto inst = mod.insts[i];

            // Remap operands to new instruction indices
            for (auto& op : inst.operands) {
                if (op.is_inst()) {
                    op.id = value_map[op.id];
                }
            }

            value_map[i] = new_insts.size();
            new_insts.push_back(inst);
        }
    }

    mod.insts = std::move(new_insts);
}

}
