#pragma once

#include "kestrel/kestrel.h"

#include <amdgpu.h>
#include <amdgpu_drm.h>

#include "cmdstream.h"
#include "hw/gpuinfo.h"
#include "shader.h"

#include "common.h"

#include <vector>

struct DeviceImpl {
    int fd;
    amdgpu_device_handle amd_handle;

    amdgpu_bo_list_handle global_residency_list;
    std::vector<amdgpu_bo_handle> all_bos;
    bool residency_dirty = true;

    uint32_t num_queues[AMDGPU_HW_IP_NUM];
    GpuInfo info;
};

struct QueueImpl {
    amdgpu_context_handle ctx_handle;
    DeviceImpl *dev;
    KesQueueType type;

    uint32_t hw_ip_type;

    CommandRing *cmd_ring;
};

struct CommandListImpl {
    QueueImpl *queue;
    CommandStream cs;

    struct {
        Shader *shader;
    } state;
};

struct SemaphoreImpl {
    amdgpu_device_handle dev_handle;
    uint32_t syncobj_handle;
};

extern "C" {
KesDevice amdgpu_create(int drm_fd);
void amdgpu_destroy(KesDevice);
struct KesAllocation amdgpu_malloc(KesDevice, size_t size, size_t align, KesMemory);
void amdgpu_free(KesDevice, struct KesAllocation *);

KesQueue amdgpu_create_queue(KesDevice, KesQueueType);
void amdgpu_destroy_queue(KesQueue);

KesCommandList amdgpu_start_recording(KesQueue);

void amdgpu_submit(KesQueue, KesCommandList, KesSemaphore, uint64_t);

void amdgpu_cmd_memset(KesCommandList, kes_gpuptr_t addr, size_t size, uint32_t value);
void amdgpu_cmd_memcpy(KesCommandList, kes_gpuptr_t dst, kes_gpuptr_t src, size_t size);
void amdgpu_cmd_write_timestamp(KesCommandList, kes_gpuptr_t addr);

void amdgpu_cmd_signal_after(KesCommandList, KesStage before, kes_gpuptr_t addr, uint64_t value, KesSignal);
void amdgpu_cmd_wait_before(KesCommandList, KesStage after, kes_gpuptr_t addr, uint64_t value, KesOp, KesHazardFlags, uint64_t mask);
void amdgpu_cmd_dispatch(KesCommandList pcl, kes_gpuptr_t data_ptr, uint32_t x, uint32_t y, uint32_t z);
void amdgpu_cmd_dispatch_indirect(KesCommandList pcl, kes_gpuptr_t data_ptr, kes_gpuptr_t indirect_addr);

KesSemaphore amdgpu_create_semaphore(KesDevice, uint64_t);
int amdgpu_wait_semaphore(KesSemaphore, uint64_t);

KesShader amdgpu_create_shader(KesDevice device, void *module);
void amdgpu_bind_shader(KesCommandList command_list, KesShader shader);
}

void device_register_allocation(DeviceImpl *impl, amdgpu_bo_handle bo);

uint32_t hw_ip_type_from_queue_type(KesQueueType qt);
