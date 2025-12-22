#include "amdgpu/sdma_encoder.h"
#include "vektor/vektor.h"
#include "vektor_impl.h"

#include "beta.h"
#include <fmt/format.h>

namespace vektor {

void memset_transfer(CommandListImpl *impl, gpuptr_t addr, std::size_t size, uint32_t value) {
    assert(impl->queue->type == QueueType::Transfer, "memset_transfer requires queue of Transfer type");

    SDMAEncoder enc(impl->queue->dev->info, impl->cs);

    while (size > 0) {
        uint64_t bytes_written = enc.constant_fill(addr, size, value);
        size -= bytes_written;
        addr += bytes_written;
    }
}

void memset(CommandList pcl, gpuptr_t addr, std::size_t size, uint32_t value) {
    auto *cl = (CommandListImpl *)pcl;

    switch(cl->queue->type) {
    case QueueType::Transfer:
        memset_transfer(cl, addr, size, value);
        break;
    default:
        not_implemented("vektor::memset not implemented for queue type: {}", fmt::underlying(cl->queue->type));
    }
}

}
