#pragma once

#include "kestrel.h"
#include "kestrel/kestrel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Holds the set of functions exported by the driver, as well as the version.
 * @sa kes_drv_interface
 */
struct KesDriverFuncs {
    /**
     * Version
     */
    uint32_t version;
    KesDevice          (*fn_create)(int drm_fd);
    void               (*fn_destroy)(KesDevice);
    struct KesAllocation (*fn_malloc)(KesDevice, size_t size, size_t align, enum KesMemory);
    void               (*fn_free)(KesDevice, struct KesAllocation *);
    KesQueue           (*fn_create_queue)(KesDevice, enum KesQueueType);
    void               (*fn_destroy_queue)(KesQueue);
    KesCommandList     (*fn_start_recording)(KesQueue);
    void               (*fn_submit)(KesQueue, KesCommandList);
    void               (*fn_cmd_memset)(KesCommandList, kes_gpuptr_t addr, size_t size, uint32_t value);
    void               (*fn_cmd_memcpy)(KesCommandList, kes_gpuptr_t dst, kes_gpuptr_t src, size_t size);
    void               (*fn_cmd_write_timestamp)(KesCommandList, kes_gpuptr_t addr);
    void               (*fn_cmd_signal_after)(KesCommandList, enum KesStage before, kes_gpuptr_t addr, uint64_t value, enum KesSignal);
    void               (*fn_cmd_wait_before)(KesCommandList, enum KesStage after, kes_gpuptr_t addr, uint64_t value, enum KesOp, enum KesHazardFlags, uint64_t mask);
    void               (*fn_cmd_dispatch)(KesCommandList command_list, kes_gpuptr_t data, uint32_t x, uint32_t y, uint32_t z);
    void               (*fn_cmd_dispatch_indirect)(KesCommandList command_list, kes_gpuptr_t data, kes_gpuptr_t command_addr);
};

/**
 *
 */
void kes_drv_interface(struct KesDriverFuncs *fns);

#ifdef __cplusplus
}
#endif
