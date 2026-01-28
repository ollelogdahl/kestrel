#include "gir.h"

#include <cstring>

namespace gir {

Value Builder::i32(int32_t imm) {
    return mod.emit(Inst{
        .op = Op::Const,
        .type = Type::I32,
        .operands = {},
        .data = {.imm_i64 = imm}
    });
}

Value Builder::f32(float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(float));
    return mod.emit(Inst{
        .op = Op::Const,
        .type = Type::F32,
        .operands = {},
        .data = {.imm_i64 = (int64_t)bits}
    });
}

Value Builder::add(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Add,
        .type = Type::I32,
        .operands = {a, b}
    });
}

Value Builder::sub(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Sub,
        .type = Type::I32,
        .operands = {a, b}
    });
}

Value Builder::mul(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Mul,
        .type = Type::I32,
        .operands = {a, b}
    });
}

Value Builder::fadd(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::FAdd,
        .type = Type::F32,
        .operands = {a, b}
    });
}

Value Builder::fmul(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::FMul,
        .type = Type::F32,
        .operands = {a, b}
    });
}

Value Builder::eq(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Eq,
        .type = Type::I32,
        .operands = {a, b}
    });
}

Value Builder::lt(Value a, Value b) {
    return mod.emit(Inst{
        .op = Op::Lt,
        .type = Type::I32,
        .operands = {a, b}
    });
}

Value Builder::load(Value addr) {
    return mod.emit(Inst{
        .op = Op::Load,
        .type = Type::I32,
        .operands = {addr},
    });
}

Value Builder::load_shared(Value addr) {
    return mod.emit(Inst{
        .op = Op::LoadShared,
        .type = Type::I32,
        .operands = {addr},
    });
}

void Builder::store(Value addr, Value data) {
    mod.emit(Inst{
        .op = Op::Store,
        .type = Type::Void,
        .operands = {addr, data},
    });
}

void Builder::store_shared(Value addr, Value data) {
    mod.emit(Inst{
        .op = Op::StoreShared,
        .type = Type::Void,
        .operands = {addr, data},
    });
}

Value Builder::get_root_ptr() {
    return mod.emit(Inst{
        .op = Op::GetRootPtr,
        .type = Type::Ptr,
        .operands = {}
    });
}

Value Builder::get_local_invocation_id() {
    return mod.emit(Inst{
        .op = Op::GetLocalInvocationId,
        .type = Type::I32,
        .operands = {}
    });
}

Value Builder::get_thread_id_x() {
    return mod.emit(Inst{
        .op = Op::GetThreadIdX,
        .type = Type::I32,
        .operands = {}
    });
}

Value Builder::get_thread_id_y() {
    return mod.emit(Inst{
        .op = Op::GetThreadIdY,
        .type = Type::I32,
        .operands = {}
    });
}

Value Builder::get_thread_id_z() {
    return mod.emit(Inst{
        .op = Op::GetThreadIdZ,
        .type = Type::I32,
        .operands = {}
    });
}


Value Builder::get_workgroup_id_x() {
    return mod.emit(Inst{
        .op = Op::GetWorkgroupIdX,
        .type = Type::I32,
        .operands = {}
    });
}

Value Builder::get_workgroup_id_y() {
    return mod.emit(Inst{
        .op = Op::GetWorkgroupIdY,
        .type = Type::I32,
        .operands = {}
    });
}

Value Builder::get_workgroup_id_z() {
    return mod.emit(Inst{
        .op = Op::GetWorkgroupIdZ,
        .type = Type::I32,
        .operands = {}
    });
}

}
