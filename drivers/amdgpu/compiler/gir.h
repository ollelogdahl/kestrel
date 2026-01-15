#pragma once

#include "rdna2_asm.h"
#include <cstdint>
#include <vector>

namespace gir {
enum class Type {
    Int, Addr,
};

enum Op { ADD, SUB, LOAD_GLOBAL, STORE_GLOBAL, GET_ROOT_PTR, V_MOV_S2V, LOAD_ROOT_PTR };

struct Ref { uint32_t id; };

struct Inst {
    Op op;
    Ref dest;
    std::vector<Ref> args;
    uint32_t imm = 0;
};

struct ValueMeta {
    bool is_uniform = false;
    Type type;
    uint32_t phys_reg = 0xFFFFFFFF;
    uint32_t last_use = 0;
};

class IRModule {
public:
    std::vector<ValueMeta> values;
    std::vector<Inst> insts;

    inline Ref make_value(Type type) {
        values.push_back({false, type, 0xFFFFFFFF, 0});
        return { (uint32_t)values.size() - 1 };
    }
};

class Builder {
public:
    Builder(IRModule& m) : mod(m) {}

    Ref iadd(Ref a, Ref b);

    Ref load_root_ptr();

    Ref load_global(Ref addr, uint32_t offset);

    void store_global(Ref addr, Ref data, uint32_t offset);
private:
    IRModule& mod;
};

};
