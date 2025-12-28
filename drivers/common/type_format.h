#pragma once

#include "kestrel/kestrel.h"

#include <string>
inline std::string format_as(KesQueueType qt) {
    switch(qt) {
    case KesQueueTypeGraphics: return "graphics";
    case KesQueueTypeCompute: return "compute";
    case KesQueueTypeTransfer: return "transfer";
    default:
        return "unknown";
    }
}

inline std::string format_as(KesSignal sig) {
    switch(sig) {
    case KesSignalAtomicSet: return "atomic_set";
    case KesSignalAtomicMax: return "atomic_max";
    case KesSignalAtomicOr:  return "atomic_or";
    default:
        return "unknown";
    }
}

inline std::string format_as(KesOp op) {
    switch(op) {
    case KesOpNever: return "never";
    case KesOpLess: return "less";
    case KesOpEqual: return "equal";
    default:
        return "unknown";
    }
}
