#pragma once

#include <cstdint>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t kes_gpuptr_t;
typedef void *KesDevice;
typedef void *KesQueue;
typedef void *KesCommandList;

struct KesAllocation {
    void *cpu;
    kes_gpuptr_t gpu;
    size_t size;
    void *_internal;
};

enum KesMemory {
    KesMemoryDefault,
    KesMemoryGpu,
    KesMemoryReadback
};

enum KesHazardFlags {
    KesHazardFlagsNone = 0,
    KesHazardFlagsDrawArguments = 1 << 0,
    KesHazardFlagsDescriptors = 1 << 1,
};

enum KesOp {
    KesOpNever,
    KesOpLess,
    KesOpEqual
};

enum KesSignal {
    KesSignalAtomicSet,
    KesSignalAtomicMax,
    KesSignalAtomicOr,
};

enum KesStage {
    KesStageTransfer,
    KesStageCompute,
    KesStageRasterColorOut,
    KesStagePixelShader,
    KesStageVertexShader
};

enum KesQueueType {
    KesQueueTypeGraphics,
    KesQueueTypeCompute,
    KesQueueTypeTransfer
};

KesDevice kes_create();
void kes_destroy(KesDevice);

struct KesAllocation kes_malloc(KesDevice, size_t size, size_t align, KesMemory);
void kes_free(KesDevice, struct KesAllocation *);

KesQueue kes_create_queue(KesDevice, KesQueueType);
void kes_destroy_queue(KesQueue);

KesCommandList kes_start_recording(KesQueue);

void kes_submit(KesQueue, KesCommandList);

void kes_cmd_memset(KesCommandList, kes_gpuptr_t addr, size_t size, uint32_t value);
void kes_cmd_write_timestamp(KesCommandList, kes_gpuptr_t addr);

void kes_cmd_signal_after(KesCommandList, KesStage before, kes_gpuptr_t addr, uint64_t value, KesSignal);
void kes_cmd_wait_before(KesCommandList, KesStage after, kes_gpuptr_t addr, uint64_t value, KesOp, KesHazardFlags);

#ifdef __cplusplus
}
#endif
