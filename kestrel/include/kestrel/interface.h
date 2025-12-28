#pragma once

#include "kestrel/kestrel.h"

#ifdef __cplusplus
extern "C" {
#endif

struct KesDriverFuncs {
    uint32_t version;
    KesDevice (*fn_create)(int drm_fd);
    typeof(kes_destroy) *fn_destroy;
    typeof(kes_malloc) *fn_malloc;
    typeof(kes_free) *fn_free;
    typeof(kes_create_queue) *fn_create_queue;
    typeof(kes_destroy_queue) *fn_destroy_queue;
    typeof(kes_start_recording) *fn_start_recording;
    typeof(kes_submit) *fn_submit;
    typeof(kes_cmd_memset) *fn_cmd_memset;
    typeof(kes_cmd_write_timestamp) *fn_cmd_write_timestamp;
    typeof(kes_cmd_signal_after) *fn_cmd_signal_after;
    typeof(kes_cmd_wait_before) *fn_cmd_wait_before;
};

void kes_drv_interface(struct KesDriverFuncs *fns);

#ifdef __cplusplus
}
#endif
