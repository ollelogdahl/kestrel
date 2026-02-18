#pragma once

#include <cstdint>

#include "kestrel/gir.h"

struct CompileShaderInfo {
    uint32_t num_user_sgprs;
};

void rdna2_compile(gir::Module &mod, CompileShaderInfo shdr, void *write_ptr, uint64_t base_addr);
