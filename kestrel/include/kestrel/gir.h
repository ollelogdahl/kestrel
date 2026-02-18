#pragma once

/*
 * GIR is the IR language used.
 *
 * @NOTE:
 * This will actually NOT be exposed in the final API. I think! I want some kind of
 * spir-v compilation or otherwise. Not completely sure yet.
 */

#include <cstdint>
#include <vector>
#include <functional>
#include <string_view>
#include <cstring>

#define GIR_VERSION "v1.1"

namespace gir {

enum class Type {
    Void,
    I32,
    F32,
    Ptr,
};

struct Value {
    uint32_t id;

    bool is_inst() const { return id != ~0u; }
};

enum class Op {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    FAdd,
    FSub,
    FMul,
    FDiv,
    And,
    Or,
    Xor,
    Shl,
    Shr,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    Load,
    LoadShared,
    Store,
    StoreShared,
    Const,
    RootPtr,
    LocalInvocationIdX,
    LocalInvocationIdY,
    LocalInvocationIdZ,
    LocalInvocationIndex,
    WorkgroupIdX,
    WorkgroupIdY,
    WorkgroupIdZ,
    SubgroupId,
    SubgroupSize,
    NumSubgroups,
    GlobalInvocationIdX,
    GlobalInvocationIdY,
    GlobalInvocationIdZ,
    GlobalInvocationIndex,
    WorkgroupBarrier,
    SubgroupBarrierInit,
    SubgroupBarrierWait,
    SubgroupBarrierSignal,
    BackendIntrinsic,
};

struct Inst {
    Op op;
    Type type;
    std::vector<Value> operands;

    // only for BackendIntrinsic
    uint32_t intrinsic_id;

    union {
        int64_t imm_i64;

        struct {
            uint32_t resource_id;
        } barrier_data;
    } data;

    struct {
        bool is_uniform = false;
        uint32_t phys_reg = ~0u;
        uint32_t last_use = 0xFFFFFFFF;
    } meta;
};

class Module {
public:
    std::vector<Inst> insts;

    uint32_t workgroup_size_x = 1;
    uint32_t workgroup_size_y = 1;
    uint32_t workgroup_size_z = 1;

    Value emit(Inst inst) {
        uint32_t id = insts.size();
        insts.push_back(inst);
        return Value{id};
    }

    Inst &deref(Value v) {
        return insts[v.id];
    }
};

class Builder {
public:
    Builder(Module& m) : mod(m) {}

    Value i32(int32_t imm);
    Value f32(float f);

    Value add(Value a, Value b);
    Value sub(Value a, Value b);
    Value mul(Value a, Value b);

    Value fadd(Value a, Value b);
    Value fmul(Value a, Value b);

    Value eq(Value a, Value b);
    Value lt(Value a, Value b);

    Value load(Value addr);
    Value load_shared(Value addr);
    void store(Value addr, Value data);
    void store_shared(Value addr, Value data);

    Value root_ptr();

    Value local_invocation_id_x();
    Value local_invocation_id_y();
    Value local_invocation_id_z();
    Value local_invocation_index();

    Value workgroup_id_x();
    Value workgroup_id_y();
    Value workgroup_id_z();

    Value subgroup_id();
    Value subgroup_size();
    Value num_subgroups();

    void workgroup_barrier();

    void subgroup_barrier_init(uint32_t resource_id, Value v);
    void subgroup_barrier_wait(uint32_t resource_id);
    void subgroup_barrier_signal(uint32_t resource_id);

protected:
    Module& mod;
};

std::string dump_module(Module &mod, std::function<std::string_view(uint32_t)> backend_intrinsic_to_string);

void pass_normalize(Module& mod);
void pass_eliminate_dead_code(Module &mod);

// builder impl
inline Value Builder::i32(int32_t imm) {
    return mod.emit(Inst{
        .op = Op::Const,
        .type = Type::I32,
        .operands = {},
        .data = {.imm_i64 = imm}
    });
}

inline Value Builder::f32(float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(float));
    return mod.emit(Inst{
        .op = Op::Const,
        .type = Type::F32,
        .operands = {},
        .data = {.imm_i64 = (int64_t)bits}
    });
}

inline Value Builder::add(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Add,
        .type = Type::I32,
        .operands = {a, b}
    });
}

inline Value Builder::sub(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Sub,
        .type = Type::I32,
        .operands = {a, b}
    });
}

inline Value Builder::mul(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Mul,
        .type = Type::I32,
        .operands = {a, b}
    });
}

inline Value Builder::fadd(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::FAdd,
        .type = Type::F32,
        .operands = {a, b}
    });
}

inline Value Builder::fmul(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::FMul,
        .type = Type::F32,
        .operands = {a, b}
    });
}

inline Value Builder::eq(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Eq,
        .type = Type::I32,
        .operands = {a, b}
    });
}

inline Value Builder::lt(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Lt,
        .type = Type::I32,
        .operands = {a, b}
    });
}

inline Value Builder::load(Value addr) {
    return mod.emit(Inst{
        .op = Op::Load,
        .type = Type::I32,
        .operands = {addr},
    });
}

inline Value Builder::load_shared(Value addr) {
    return mod.emit(Inst{
        .op = Op::LoadShared,
        .type = Type::I32,
        .operands = {addr},
    });
}

inline void Builder::store(Value addr, Value data) {
    mod.emit(Inst{
        .op = Op::Store,
        .type = Type::Void,
        .operands = {addr, data},
    });
}

inline void Builder::store_shared(Value addr, Value data) {
    mod.emit(Inst{
        .op = Op::StoreShared,
        .type = Type::Void,
        .operands = {addr, data},
    });
}

inline Value Builder::root_ptr() {
    return mod.emit(Inst{
        .op = Op::RootPtr,
        .type = Type::Ptr,
        .operands = {}
    });
}

inline Value Builder::local_invocation_id_x() {
    return mod.emit(Inst{
        .op = Op::LocalInvocationIdX,
        .type = Type::I32,
        .operands = {}
    });
}

inline Value Builder::local_invocation_id_y() {
    return mod.emit(Inst{
        .op = Op::LocalInvocationIdY,
        .type = Type::I32,
        .operands = {}
    });
}

inline Value Builder::local_invocation_id_z() {
    return mod.emit(Inst{
        .op = Op::LocalInvocationIdZ,
        .type = Type::I32,
        .operands = {}
    });
}

inline Value Builder::local_invocation_index() {
    return mod.emit(Inst{
        .op = Op::LocalInvocationIndex,
        .type = Type::I32,
        .operands = {}
    });
}


inline Value Builder::workgroup_id_x() {
    return mod.emit(Inst{
        .op = Op::WorkgroupIdX,
        .type = Type::I32,
        .operands = {}
    });
}

inline Value Builder::workgroup_id_y() {
    return mod.emit(Inst{
        .op = Op::WorkgroupIdY,
        .type = Type::I32,
        .operands = {}
    });
}

inline Value Builder::workgroup_id_z() {
    return mod.emit(Inst{
        .op = Op::WorkgroupIdZ,
        .type = Type::I32,
        .operands = {}
    });
}

inline Value Builder::subgroup_id() {
    Inst inst;
    inst.op = Op::SubgroupId;
    inst.type = Type::I32;
    inst.operands = {};
    return mod.emit(inst);
}

inline Value Builder::subgroup_size() {
    Inst inst;
    inst.op = Op::SubgroupSize;
    inst.type = Type::I32;
    inst.operands = {};
    return mod.emit(inst);
}

inline Value Builder::num_subgroups() {
    Inst inst;
    inst.op = Op::NumSubgroups;
    inst.type = Type::I32;
    inst.operands = {};
    return mod.emit(inst);
}

inline void Builder::workgroup_barrier() {
    Inst inst;
    inst.op = Op::WorkgroupBarrier;
    inst.type = Type::Void;
    inst.operands = {};
    mod.emit(inst);
}

inline void Builder::subgroup_barrier_init(uint32_t resource_id, Value v) {
    Inst inst;
    inst.op = Op::SubgroupBarrierInit;
    inst.type = Type::Void;
    inst.data.barrier_data.resource_id = resource_id;
    inst.operands = {v};
    mod.emit(inst);
}

inline void Builder::subgroup_barrier_wait(uint32_t resource_id) {
    Inst inst;
    inst.op = Op::SubgroupBarrierWait;
    inst.type = Type::Void;
    inst.data.barrier_data.resource_id = resource_id;
    inst.operands = {};
    mod.emit(inst);
}

inline void Builder::subgroup_barrier_signal(uint32_t resource_id) {
    Inst inst;
    inst.op = Op::SubgroupBarrierSignal;
    inst.type = Type::Void;
    inst.data.barrier_data.resource_id = resource_id;
    inst.operands = {};
    mod.emit(inst);
}

}
