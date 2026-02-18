#include "impl.h"
#include "kestrel/interface.h"

#define KESDRV_AMDGPU_VERSION "amdgpu-0.0.1"
#define KESDRV_AMDGPU_VERSION_NUM 1

API_EXPORT void kes_drv_interface(struct KesDriverFuncs *fns) {
    fns->version = KESDRV_AMDGPU_VERSION_NUM;
    fns->fn_create = amdgpu_create;
    fns->fn_destroy = amdgpu_destroy;
    fns->fn_malloc = amdgpu_malloc;
    fns->fn_free = amdgpu_free;
    fns->fn_create_queue = amdgpu_create_queue;
    fns->fn_destroy_queue = amdgpu_destroy_queue;
    fns->fn_start_recording = amdgpu_start_recording;
    fns->fn_submit = amdgpu_submit;
    fns->fn_cmd_memset = amdgpu_cmd_memset;
    fns->fn_cmd_memcpy = amdgpu_cmd_memcpy;
    fns->fn_cmd_write_timestamp = amdgpu_cmd_write_timestamp;
    fns->fn_cmd_signal_after = amdgpu_cmd_signal_after;
    fns->fn_cmd_wait_before = amdgpu_cmd_wait_before;
    fns->fn_cmd_dispatch = amdgpu_cmd_dispatch;
    fns->fn_cmd_dispatch_indirect = amdgpu_cmd_dispatch_indirect;
    fns->fn_create_semaphore = amdgpu_create_semaphore;
    fns->fn_wait_semaphore = amdgpu_wait_semaphore;
    fns->fn_create_shader = amdgpu_create_shader;
    fns->fn_bind_shader = amdgpu_bind_shader;
}
