#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <string_view>

#define GIR_VERSION "v1"

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
    Store,
    Const,
    GetRootPtr,
    GetLocalInvocationId,
    GetThreadIdX,
    GetThreadIdY,
    GetThreadIdZ,
    GetWorkgroupIdX,
    GetWorkgroupIdY,
    GetWorkgroupIdZ,
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
    void store(Value addr, Value data);

    Value get_root_ptr();

    Value get_local_invocation_id();
    Value get_thread_id_x();
    Value get_thread_id_y();
    Value get_thread_id_z();

    Value get_workgroup_id_x();
    Value get_workgroup_id_y();
    Value get_workgroup_id_z();

protected:
    Module& mod;
};

std::string dump_module(Module &mod, std::function<std::string_view(uint32_t)> backend_intrinsic_to_string);

void pass_normalize(Module& mod);
void pass_eliminate_dead_code(Module &mod);

};
