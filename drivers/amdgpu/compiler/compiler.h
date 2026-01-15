#pragma once

#include "gir/gir.h"

void rdna2_compile(gir::Module &mod, void *write_ptr, uint64_t base_addr);
