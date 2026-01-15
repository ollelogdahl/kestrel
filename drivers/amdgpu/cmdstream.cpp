#include "cmdstream.h"

#include "impl.h"
#include "beta.h"

KesSemaphore amdgpu_create_semaphore(KesDevice pd, uint64_t initial_value) {
    auto *dev = reinterpret_cast<DeviceImpl *>(pd);
    auto *sem = new SemaphoreImpl();
    sem->dev_handle = dev->amd_handle;

    int r = amdgpu_cs_create_syncobj2(dev->amd_handle, DRM_SYNCOBJ_CREATE_SIGNALED, &sem->syncobj_handle);

    // Set the initial timeline point
    if (initial_value > 0) {
        amdgpu_cs_syncobj_timeline_signal(dev->amd_handle, &sem->syncobj_handle, &initial_value, 1);
    }

    return sem;
}

int amdgpu_wait_semaphore(KesSemaphore ps, uint64_t value) {
    return 0;
    /*
    auto *sem = reinterpret_cast<SemaphoreImpl *>(ps);

    uint32_t wait_flags = DRM_SYNCOBJ_WAIT_FLAGS_WAIT_ALL | DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT;

    int r = amdgpu_cs_syncobj_timeline_wait(sem->dev_handle, &sem->syncobj_handle, &value, 1,
                                            1000000000, wait_flags, nullptr);
    // In a real driver, you'd check the return code here for -ECANCELED (GPU Reset)
    return r;
     */
}


void CommandStream::emit(uint32_t x) {
    assert(cursor < end, "commandstream emit out of bounds: {}-{} {}", (void *)start, (void *)end, (void *)cursor);
    *cursor++ = x;
}

CommandRing::CommandRing(DeviceImpl *dev, amdgpu_context_handle ctx, uint32_t ip_type, Config cfg)
    : m_dev(dev), m_ctx(ctx), m_ip_type(ip_type), m_cfg(cfg) {

    amdgpu_bo_alloc_request req = {
        .alloc_size = m_cfg.ring_size_bytes,
        .phys_alignment = 4096,
        .preferred_heap = AMDGPU_GEM_DOMAIN_GTT,
        .flags = AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED | AMDGPU_GEM_CREATE_UNCACHED // Or WC
    };

    amdgpu_cs_create_syncobj2(dev->amd_handle, DRM_SYNCOBJ_CREATE_SIGNALED, &m_queue_syncobj);

    amdgpu_bo_alloc(m_dev->amd_handle, &req, &m_bo_handle);

    void* ptr;
    amdgpu_bo_cpu_map(m_bo_handle, &ptr);
    m_cpu_map = static_cast<uint32_t*>(ptr);

    amdgpu_va_handle va_handle;
    amdgpu_va_range_alloc(m_dev->amd_handle, amdgpu_gpu_va_range_general, m_cfg.ring_size_bytes, 1, 0, &m_gpu_va, &va_handle, 0);
    amdgpu_bo_va_op(m_bo_handle, 0, m_cfg.ring_size_bytes, m_gpu_va, AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_EXECUTABLE, AMDGPU_VA_OP_MAP);

    log("command ring alloc: {} {}", (void *)m_cpu_map, (void *)m_gpu_va);

    device_register_allocation(m_dev, m_bo_handle);
}

CommandStream CommandRing::begin_recording() {
    uint32_t stream_dw = m_cfg.stream_size_bytes / 4;

    if (m_write_cursor_dw + stream_dw > (m_cfg.ring_size_bytes / 4)) {
        m_write_cursor_dw = 0;
    }

    wait_for_space(m_write_cursor_dw);

    CommandStream cs;
    cs.cursor = m_cpu_map + m_write_cursor_dw;
    cs.end    = cs.cursor + stream_dw;
    cs.gpu_va_start = m_gpu_va + (m_write_cursor_dw * 4);

    return cs;
}

// @todo: this can support multiple semaphores in a single chunk.
void *alloc_timeline_syncobj_chunk(drm_amdgpu_cs_chunk *chunk, uint32_t syncobj, uint64_t point, uint32_t chunk_id) {
    auto count = 1;
    auto sems = (drm_amdgpu_cs_chunk_syncobj *)malloc(sizeof(drm_amdgpu_cs_chunk_syncobj) * count);

    sems[0].handle = syncobj;
    sems[0].flags = DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT;
    sems[0].point = point;

    chunk->chunk_id = chunk_id;
    chunk->length_dw = sizeof(drm_amdgpu_cs_chunk_syncobj) / 4 * count;
    chunk->chunk_data = (uint64_t)(uintptr_t)sems;

    return sems;
}

void CommandRing::submit(CommandStream& cs, SemaphoreImpl *sem, uint64_t value) {
    // @todo: we currently ignore user sem.

    uint32_t start_dw = cs.end - (m_cfg.stream_size_bytes / 4) - m_cpu_map;
    start_dw = (reinterpret_cast<uint8_t*>(cs.end) - reinterpret_cast<uint8_t*>(m_cpu_map) - m_cfg.stream_size_bytes) / 4;

    uint32_t count_dw = cs.cursor - (cs.end - (m_cfg.stream_size_bytes / 4));

    auto has_user_sem = sem != nullptr;

    auto chunks = (drm_amdgpu_cs_chunk *)malloc(sizeof(drm_amdgpu_cs_chunk) * 4);
    auto chunk_data = (drm_amdgpu_cs_chunk_data *)malloc(sizeof(drm_amdgpu_cs_chunk_data) * 1);

    auto num_chunks = 1;
    {
        chunks[0].chunk_id = AMDGPU_CHUNK_ID_IB;
        chunks[0].length_dw = sizeof(drm_amdgpu_cs_chunk_ib) / 4;
        chunks[0].chunk_data = (uint64_t)(uintptr_t)&chunk_data[0];

        chunk_data[0].ib_data._pad = 0;
        chunk_data[0].ib_data.va_start = cs.gpu_va_start;
        chunk_data[0].ib_data.ib_bytes = count_dw * 4;
        chunk_data[0].ib_data.ip_type = m_ip_type;
        chunk_data[0].ib_data.ip_instance = 0;
        chunk_data[0].ib_data.ring = 0;
        chunk_data[0].ib_data.flags = 0;
    }

    if (false) {
        chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_BO_HANDLES;
        chunks[num_chunks].length_dw = sizeof(struct drm_amdgpu_bo_list_in) / 4;
        chunks[num_chunks].chunk_data = (uintptr_t)m_dev->global_residency_list;
        num_chunks++;
    }

    // create a wait block for the last submission.
    alloc_timeline_syncobj_chunk(&chunks[num_chunks], m_queue_syncobj, m_timeline_counter, AMDGPU_CHUNK_ID_SYNCOBJ_TIMELINE_WAIT);
    num_chunks++;

    m_timeline_counter++;

    // create a signal block for the finishing of this submission.
    alloc_timeline_syncobj_chunk(&chunks[num_chunks], m_queue_syncobj, m_timeline_counter, AMDGPU_CHUNK_ID_SYNCOBJ_TIMELINE_SIGNAL);
    num_chunks++;

    auto r = amdgpu_cs_submit_raw2(m_dev->amd_handle, m_ctx, 0, num_chunks, chunks, nullptr);
    if (r != 0) {
        warn("submit: failed with error: {}", r);
    }
}

void CommandRing::wait_for_space(uint32_t target_dw) {
    /*
    while (!m_history.empty()) {
        auto& oldest = m_history.front();

        // If target overlaps with the oldest pending submission
        if (target_dw >= oldest.start_dw && target_dw < oldest.end_dw) {
            uint32_t expired = 0;
            amdgpu_cs_wait_fences(&oldest.fence, 1, true, 1000000000, &expired, nullptr);
            m_history.pop_front();
        } else {
            break;
        }
    }
    */
}

CommandRing::~CommandRing() {
    amdgpu_bo_cpu_unmap(m_bo_handle);
    amdgpu_bo_free(m_bo_handle);
}
