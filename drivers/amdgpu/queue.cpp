#include "kestrel/kestrel.h"
#include "impl.h"

#include <cstdint>
#include <vector>

uint32_t hw_ip_type_from_queue_type(KesQueueType qt) {
    switch(qt) {
    case KesQueueTypeGraphics: return AMDGPU_HW_IP_GFX;
    case KesQueueTypeCompute: return AMDGPU_HW_IP_COMPUTE;
    case KesQueueTypeTransfer: return AMDGPU_HW_IP_DMA;
    default:
        not_implemented("no HW_IP type picked for queue type: {}", qt);
    }
}

KesQueue amdgpu_create_queue(KesDevice pd, KesQueueType qt) {
    auto *dev = reinterpret_cast<DeviceImpl *>(pd);

    auto hw_ip_type = hw_ip_type_from_queue_type(qt);

    assert(dev->num_queues[hw_ip_type] < dev->info.ip[hw_ip_type].num_queues,
        "amdgpu_create_queue: too many queues created for type: {} (created: {}, max: {})",
        qt, dev->num_queues[hw_ip_type], dev->info.ip[hw_ip_type].num_queues);

    auto queue = new QueueImpl;
    queue->dev = dev;
    queue->type = qt;
    queue->hw_ip_type = hw_ip_type;

    // @todo: consider creating ctx at device initialization?
    int r = amdgpu_cs_ctx_create(dev->amd_handle, &queue->ctx_handle);
    if (r != 0) {
        delete queue;
        return nullptr;
    }

    dev->num_queues[hw_ip_type]++;

    // @todo: cleanup: remove this fkn pointer; shit stuff we don't need!
    auto conf = CommandRing::Config{};
    queue->cmd_ring = new CommandRing(dev->amd_handle, queue->ctx_handle, queue->hw_ip_type, conf);

    return queue;
}

void amdgpu_destroy_queue(KesQueue pq) {
    auto *queue = reinterpret_cast<QueueImpl *>(pq);
    // @todo: actually delete queue.

    auto hw_ip_type = hw_ip_type_from_queue_type(queue->type);
    queue->dev->num_queues[hw_ip_type]--;
}

KesCommandList amdgpu_start_recording(KesQueue pq) {
    auto *queue = reinterpret_cast<QueueImpl *>(pq);
    auto cl = new CommandListImpl;

    cl->queue = queue;
    cl->cs = queue->cmd_ring->begin_recording();

    return cl;
}

// @todo: add support for semaphore or other synchronization.
void amdgpu_submit(KesQueue pq, KesCommandList pcl) {
    auto *queue = reinterpret_cast<QueueImpl *>(pq);
    auto *cl = reinterpret_cast<CommandListImpl *>(pcl);
    assert(cl->queue == queue, "submit: commandlist from foreign queue");

    queue->cmd_ring->submit(cl->cs); //, semaphore, value);

    // @todo: to free commandlist, we want to be sure that it is no longer mapped and stuff.
    // then, we can freely-free it. But i think this needs some deferred-cleanup, as
    // the data is on GTT so we cannot just let the CPU start using the range again.
    //
    // think about this.
}
