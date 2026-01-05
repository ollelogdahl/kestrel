#pragma once

#include <kestrel/kestrel.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <vector>
#include <chrono>
#include <thread>

struct DeviceGuard {
    KesDevice dev;

    DeviceGuard() : dev(kes_create()) {
        REQUIRE(dev != nullptr);
    }

    ~DeviceGuard() {
        if (dev) kes_destroy(dev);
    }

    DeviceGuard(const DeviceGuard&) = delete;
    DeviceGuard& operator=(const DeviceGuard&) = delete;

    KesAllocation alloc(size_t size, size_t align = 4, KesMemory type = KesMemoryDefault) {
        return kes_malloc(dev, size, align, type);
    }
};

struct TimestampReader {
    KesAllocation alloc;

    TimestampReader(KesDevice dev, size_t count = 8) {
        alloc = kes_malloc(dev, sizeof(uint64_t) * count, 8, KesMemoryDefault);
    }

    uint64_t get(size_t index) const {
        return ((uint64_t *)alloc.cpu)[index];
    }

    kes_gpuptr_t gpu_ptr(size_t index = 0) const {
        return alloc.gpu + index * sizeof(uint64_t);
    }

    // Check ordering: timestamps should be non-decreasing
    void check_ordering(std::vector<size_t> indices) const {
        for (size_t i = 1; i < indices.size(); i++) {
            uint64_t prev = get(indices[i-1]);
            uint64_t curr = get(indices[i]);
            INFO("Timestamp[" << indices[i-1] << "] = " << prev);
            INFO("Timestamp[" << indices[i] << "] = " << curr);
            REQUIRE(prev <= curr);
        }
    }
};

// CPU-side wait for GPU completion (for testing)
inline void busy_wait_for_value(void* ptr, uint64_t expected,
                               std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    auto start = std::chrono::steady_clock::now();
    while (*static_cast<volatile uint64_t*>(ptr) != expected) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            FAIL("Timeout waiting for GPU value");
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}
