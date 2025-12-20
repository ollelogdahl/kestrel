#include "vektor/vektor.h"
#include "vektor_impl.h"
#include <cstdint>
#include <vector>

namespace vektor {

struct QueueImpl {
    amdgpu_context_handle ctx_handle;
    uint32_t ctx_id;
};

struct CommandListImpl {
    std::vector<uint32_t> commands;

};

Queue create_queue(Device pd) {
    auto *dev = (DeviceImpl *)pd;

    auto queue = new QueueImpl;

    int r = amdgpu_cs_ctx_create(dev->amd_handle, &queue->ctx_handle);
    if (r != 0) {
        delete queue;
        return nullptr;
    }

    drm_amdgpu_ctx_in ctx_in = {};
    ctx_in.op = AMDGPU_CTX_OP_ALLOC_CTX;

    drm_amdgpu_ctx ctx_args = {};
    ctx_args.in = ctx_in;

    // r = drmCommandWriteRead(dev->fd, DRM_AMDGPU_CTX, &ctx_args, sizeof(ctx_args));
    // if (r == 0) {
    //     queue->ctx_id = ctx_args.out.alloc.ctx_id;
    // }

    return queue;
}

CommandList start_recording(Queue pq) {
    auto cl = new CommandListImpl;

    return nullptr;
}

void submit(Queue pq, CommandList pcl) {

}

}
