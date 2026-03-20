#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "tlsf.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Basic allocation and free", "[tlsf]") {
    TlsfAllocator alloc(1024);

    TlsfAllocator::Allocation a;

    REQUIRE(alloc.allocate(128, 8, a));
    REQUIRE(a.offset == 0);
    REQUIRE(a.size >= 128);

    alloc.free(a);

    // Should reuse same space
    TlsfAllocator::Allocation b;

    REQUIRE(alloc.allocate(128, 8, b));
    REQUIRE(b.offset == 0);
}

TEST_CASE("Split and merge behavior", "[tlsf]") {
    TlsfAllocator alloc(2048);

    TlsfAllocator::Allocation a1, a2;

    REQUIRE(alloc.allocate(256, 8, a1));
    REQUIRE(alloc.allocate(256, 8, a2));

    alloc.free(a1);
    alloc.free(a2);

    TlsfAllocator::Allocation a3;

    REQUIRE(alloc.allocate(1024, 8, a3));
    REQUIRE(a3.offset == 0);
}

TEST_CASE("Alignment correctness", "[tlsf]") {
    TlsfAllocator alloc(16000);

    for (uint64_t align = 1; align <= 128; align <<= 1) {
        TlsfAllocator::Allocation a;

        REQUIRE(alloc.allocate(100, align, a));
        REQUIRE(a.offset % align == 0);

        alloc.free(a);
    }
}

TEST_CASE("Full heap allocation", "[tlsf]") {
    TlsfAllocator alloc(1024);

    TlsfAllocator::Allocation a;

    REQUIRE(alloc.allocate(1024, 1, a));
    REQUIRE(a.offset == 0);

    alloc.free(a);
}

TEST_CASE("Fragmentation stress", "[tlsf]") {
    TlsfAllocator alloc(2048);

    std::vector<TlsfAllocator::Allocation> handles;

    // Fill with small blocks
    for (int i = 0; i < 16; ++i) {
        TlsfAllocator::Allocation a;
        REQUIRE(alloc.allocate(64, 8, a));
        handles.push_back(a);
    }

    // Free every other
    for (size_t i = 0; i < handles.size(); i += 2)
        alloc.free(handles[i]);

    // Try larger alloc
    TlsfAllocator::Allocation big;

    REQUIRE(alloc.allocate(256, 8, big));
}

// stress test
#include <random>

TEST_CASE("Randomized stress test", "[tlsf][fuzz]") {
    const uint64_t HEAP_SIZE = 1 << 20;
    TlsfAllocator alloc(HEAP_SIZE);
    std::byte *data = new std::byte[HEAP_SIZE];

    std::mt19937_64 rng(123456);

    struct Active {
        TlsfAllocator::Allocation a;
        uint64_t offset;
        uint64_t size;
    };

    std::vector<Active> active;

    for (int iter = 0; iter < 200000; ++iter) {
        bool doAlloc = active.empty() || (rng() % 2 == 0);

        if (doAlloc) {
            uint64_t size = (rng() % 4096) + 1;
            uint64_t align = 1ull << (rng() % 6);

            TlsfAllocator::Allocation a;

            if (alloc.allocate(size, align, a)) {
                REQUIRE((uintptr_t)a.offset % align == 0);

                for (auto& other : active) {
                    bool overlap =
                        !(a.offset + a.size <= other.offset ||
                          other.offset + other.size <= a.offset);

                    REQUIRE_FALSE(overlap);
                }

                active.push_back({a, a.offset, a.size});

                // write some data to ensure safe!
                std::memset(data + a.offset, 1, size);
            }
        } else {
            size_t idx = rng() % active.size();
            alloc.free(active[idx].a);
            active.erase(active.begin() + idx);
        }

        if (iter % 1000 == 0) {
            alloc.validate();
        }
    }
}
