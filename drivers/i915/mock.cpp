#include "impl.h"

API_EXPORT KesDevice i915_init(int drm_fd) {
    return nullptr;
}

API_EXPORT void i915_destroy(KesDevice) {
}

API_EXPORT struct KesAllocation i915_malloc(KesDevice, size_t size, size_t align, KesMemory) {
    return {};
}

API_EXPORT void i915_free(KesDevice, struct KesAllocation *) {

}

API_EXPORT KesQueue i915_create_queue(KesDevice, KesQueueType) {
    return nullptr;
}

API_EXPORT void i915_destroy_queue(KesQueue) {

}

API_EXPORT KesCommandList i915_start_recording(KesQueue) {
    return nullptr;
}

API_EXPORT void i915_submit(KesQueue, KesCommandList) {

}

API_EXPORT void i915_cmd_memset(KesCommandList, kes_gpuptr_t addr, size_t size, uint32_t value) {

}

API_EXPORT void i915_cmd_write_timestamp(KesCommandList, kes_gpuptr_t addr) {

}

API_EXPORT void i915_cmd_signal_after(KesCommandList, KesStage before, kes_gpuptr_t addr, uint64_t value, KesSignal) {

}

API_EXPORT void i915_cmd_wait_before(KesCommandList, KesStage after, kes_gpuptr_t addr, uint64_t value, KesOp, KesHazardFlags) {

}
