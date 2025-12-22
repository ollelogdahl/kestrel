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

}
