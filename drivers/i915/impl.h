#pragma once

#include "common.h"

extern "C" {
KesDevice i915_init(int drm_fd);
void i915_destroy(KesDevice);
struct KesAllocation i915_malloc(KesDevice, size_t size, size_t align, KesMemory);
void i915_free(KesDevice, struct KesAllocation *);

KesQueue i915_create_queue(KesDevice, KesQueueType);
void i915_destroy_queue(KesQueue);

KesCommandList i915_start_recording(KesQueue);

void i915_submit(KesQueue, KesCommandList);

void i915_cmd_memset(KesCommandList, kes_gpuptr_t addr, size_t size, uint32_t value);
void i915_cmd_write_timestamp(KesCommandList, kes_gpuptr_t addr);

void i915_cmd_signal_after(KesCommandList, KesStage before, kes_gpuptr_t addr, uint64_t value, KesSignal);
void i915_cmd_wait_before(KesCommandList, KesStage after, kes_gpuptr_t addr, uint64_t value, KesOp, KesHazardFlags);
}
