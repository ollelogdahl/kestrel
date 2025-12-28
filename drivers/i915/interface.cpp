#include "impl.h"

#include "kestrel/interface.h"

#define KESDRV_I915_VERSION "i915-0.0.1"
#define KESDRV_I915_VERSION_NUM 1

API_EXPORT void kes_drv_interface(struct KesDriverFuncs *fns) {
    fns->version = KESDRV_I915_VERSION_NUM;
    fns->fn_create = i915_init;
    fns->fn_destroy = i915_destroy;
    fns->fn_malloc = i915_malloc;
    fns->fn_free = i915_free;
    fns->fn_create_queue = i915_create_queue;
    fns->fn_destroy_queue = i915_destroy_queue;
    fns->fn_start_recording = i915_start_recording;
    fns->fn_submit = i915_submit;
    fns->fn_cmd_memset = i915_cmd_memset;
    fns->fn_cmd_write_timestamp = i915_cmd_write_timestamp;
    fns->fn_cmd_signal_after = i915_cmd_signal_after;
    fns->fn_cmd_wait_before = i915_cmd_wait_before;
}
