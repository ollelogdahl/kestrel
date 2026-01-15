#include "gir.h"

namespace gir {

Ref Builder::iadd(Ref a, Ref b) {
    Ref dst = mod.make_value(Type::Int);
    mod.insts.push_back({ADD, dst, {a, b}});
    return dst;
}

Ref Builder::load_root_ptr() {
    Ref dst = mod.make_value(Type::Addr);
    mod.insts.push_back({LOAD_ROOT_PTR, dst, {}});
    return dst;
}

Ref Builder::load_global(Ref addr, uint32_t offset) {
    Ref dst = mod.make_value(Type::Int);
    mod.insts.push_back({LOAD_GLOBAL, dst, {addr}, offset});
    return dst;
}

void Builder::store_global(Ref addr, Ref data, uint32_t offset) {
    mod.insts.push_back({STORE_GLOBAL, {0xFFFFFFFF}, {addr, data}, offset});
}

}
