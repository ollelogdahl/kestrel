#pragma once

#include "vektor/vektor.h"

#include <string>
namespace vektor {

inline std::string format_as(vektor::QueueType qt) {
    switch(qt) {
    case QueueType::Graphics: return "graphics";
    case QueueType::Compute: return "compute";
    case QueueType::Transfer: return "transfer";
    default:
        return "unknown";
    }
}

inline std::string format_as(vektor::Signal sig) {
    switch(sig) {
    case Signal::AtomicSet: return "atomic_set";
    case Signal::AtomicMax: return "atomic_max";
    case Signal::AtomicOr:  return "atomic_or";
    default:
        return "unknown";
    }
}

inline std::string format_as(vektor::Op op) {
    switch(op) {
    case Op::Never: return "never";
    case Op::Less: return "less";
    case Op::Equal: return "equal";
    default:
        return "unknown";
    }
}

}
