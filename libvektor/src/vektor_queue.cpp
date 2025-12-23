#include "vektor/vektor.h"
#include "vektor_impl.h"
#include "beta.h"
#include <cstdint>
#include <vector>

namespace vektor {

uint32_t hw_ip_type_from_queue_type(QueueType qt) {
    switch(qt) {
    case QueueType::Graphics: return AMDGPU_HW_IP_GFX;
    case QueueType::Compute: return AMDGPU_HW_IP_COMPUTE;
    case QueueType::Transfer: return AMDGPU_HW_IP_DMA;
    default:
        not_implemented("no HW_IP type picked for queue type: {}", qt);
    }
}

Queue create_queue(Device pd, QueueType qt) {
    auto *dev = (DeviceImpl *)pd;

    auto queue = new QueueImpl;
    queue->dev = dev;
    queue->type = qt;
    queue->hw_ip_type = hw_ip_type_from_queue_type(qt);

    // @todo: consider creating ctx at device initialization?
    int r = amdgpu_cs_ctx_create(dev->amd_handle, &queue->ctx_handle);
    if (r != 0) {
        delete queue;
        return nullptr;
    }

    // @todo: cleanup: remove this fkn pointer; shit stuff we don't need!
    auto conf = CommandRing::Config{};
    queue->cmd_ring = new CommandRing(dev->amd_handle, queue->ctx_handle, queue->hw_ip_type, conf);

    return queue;
}

CommandList start_recording(Queue pq) {
    auto *queue = (QueueImpl *)pq;
    auto cl = new CommandListImpl;

    cl->queue = queue;
    cl->cs = queue->cmd_ring->begin_recording();

    return cl;
}

void submit(Queue pq, CommandList pcl, Semaphore sem, uint64_t value) {
    auto *queue = (QueueImpl *)pq;
    auto *cl = (CommandListImpl *)pcl;
    assert(cl->queue == queue, "submit: commandlist from foreign queue");

    auto *semaphore = (SemaphoreImpl *)sem;

    queue->cmd_ring->submit(cl->cs); //, semaphore, value);

    // @todo: to free commandlist, we want to be sure that it is no longer mapped and stuff.
    // then, we can freely-free it. But i think this needs some deferred-cleanup, as
    // the data is on GTT so we cannot just let the CPU start using the range again.
    //
    // think about this.
}

}
