#pragma once

#include <cstdint>
#include <deque>

#include <amdgpu.h>
#include <amdgpu_drm.h>

struct DeviceImpl;

class CommandStream {
public:
    void emit(uint32_t);
    std::size_t size_dw() const { return cursor - start; }
private:
    uint32_t *start;
    uint32_t *end;
    uint32_t *cursor;
    uint64_t gpu_va_start;

    friend class CommandRing;
};

// @todo: think about syncronization...

class CommandRing {
public:
    struct Config {
        std::size_t ring_size_bytes = 2 * 1024 * 1024; // 2MB
        std::size_t stream_size_bytes = 128 * 1024;    // 128KB
    };

    CommandRing(DeviceImpl *dev, amdgpu_context_handle ctx, uint32_t ip_type, Config cfg);
    ~CommandRing();

    CommandRing(const CommandRing&) = delete;

    CommandStream begin_recording();
    void submit(CommandStream& cs);

private:
    struct Submission {
        uint32_t start_dw;
        uint32_t end_dw;
        uint64_t point;
    };

    void wait_for_space(uint32_t target_dw_offset);

    DeviceImpl *m_dev;
    amdgpu_context_handle m_ctx;
    uint32_t              m_ip_type;
    Config                m_cfg;

    amdgpu_bo_handle      m_bo_handle;
    uint64_t              m_gpu_va;
    uint32_t* m_cpu_map;

    uint64_t m_timeline_counter = 0;

    uint32_t              m_write_cursor_dw = 0;
    std::deque<Submission> m_history;
};
