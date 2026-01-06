#include "cmdstream.h"

#include "impl.h"
#include "beta.h"

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

void CommandRing::submit(CommandStream& cs) {
    uint32_t start_dw = cs.end - (m_cfg.stream_size_bytes / 4) - m_cpu_map;
    start_dw = (reinterpret_cast<uint8_t*>(cs.end) - reinterpret_cast<uint8_t*>(m_cpu_map) - m_cfg.stream_size_bytes) / 4;

    uint32_t count_dw = cs.cursor - (cs.end - (m_cfg.stream_size_bytes / 4));

    amdgpu_cs_ib_info ib = {};
    //ib.handle = m_bo_handle;
    ib.ib_mc_address = cs.gpu_va_start;
    ib.size = count_dw;

    auto next_point = m_timeline_counter++;

    amdgpu_cs_request req = {};
    req.ip_type = m_ip_type;
    req.number_of_ibs = 1;
    req.ibs = &ib;
    if (m_dev->residency_dirty) {
        req.resources = m_dev->global_residency_list;
        m_dev->residency_dirty = false;
    }

    auto r = amdgpu_cs_submit(m_ctx, 0, &req, 1);
    if (r != 0) {
        warn("submit failed: (ctx: {}) {}", (void *)m_ctx, r);
    }
    if (r == 0) {
        amdgpu_cs_fence fence = {};
        fence.context = m_ctx;
        fence.ip_type = m_ip_type;
        // @todo: syncronization...

        m_history.push_back({start_dw, start_dw + (uint32_t)(m_cfg.stream_size_bytes/4), next_point});
        m_write_cursor_dw += (m_cfg.stream_size_bytes / 4);
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
