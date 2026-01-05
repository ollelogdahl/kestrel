#include "test_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdlib>

TEST_CASE("Device creation and alloc") {
    DeviceGuard device;

    auto x = kes_malloc(device.dev, 256, 4, KesMemoryDefault);

    REQUIRE(x.cpu != 0);
    REQUIRE(x.gpu != 0);
}

/*
TEST_CASE("Cross-queue synchronization", "[sync][queue]") {
    DeviceGuard device;

    // Test parameters
    auto size = GENERATE(1024, 1024*1024, 10*1024*1024);

    SECTION("Transfer -> Compute sync with signal") {
        auto buffer = device.alloc(size);
        auto signal_mem = device.alloc(4);
        TimestampReader timestamps(device.dev, 4);

        auto dma = kes_create_queue(device.dev, KesQueueTypeTransfer);
        auto compute = kes_create_queue(device.dev, KesQueueTypeCompute);

        // DMA: memset + signal
        auto l1 = kes_start_recording(dma);
        kes_cmd_write_timestamp(l1, timestamps.gpu_ptr(0));
        kes_cmd_memset(l1, buffer.gpu, size, 0x42);
        kes_cmd_write_timestamp(l1, timestamps.gpu_ptr(1));
        kes_cmd_signal_after(l1, KesStageTransfer, signal_mem.gpu,
                            0x13, KesSignalAtomicSet);

        // Compute: wait + timestamp
        auto l2 = kes_start_recording(compute);
        kes_cmd_write_timestamp(l2, timestamps.gpu_ptr(2));
        kes_cmd_wait_before(l2, KesStageTransfer, signal_mem.gpu,
                           0x13, KesOpEqual, KesHazardFlagsNone, ~0);
        kes_cmd_write_timestamp(l2, timestamps.gpu_ptr(3));

        kes_submit(dma, l1);
        kes_submit(compute, l2);

        // Wait for completion
        busy_wait_for_value(signal_mem.cpu, 0x13);

        // Verify memory was set
        // REQUIRE(((uint32_t *)buffer.cpu)[0] == 0x42);
        // REQUIRE(((uint32_t *)buffer.cpu)[size-1] == 0x42);

        // Verify timestamp ordering: t0 <= t1 <= t3, t2 <= t3
        timestamps.check_ordering({0, 1, 3});
        timestamps.check_ordering({2, 3});
    }
}
*/
