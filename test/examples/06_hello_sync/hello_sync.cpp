#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

int main(void) {

    auto dev = kes_create();

    std::size_t size = 10 * 1024 * 1024;
    auto x = kes_malloc(dev, size, 4, KesMemoryDefault);
    auto y = kes_malloc(dev, 8, 4, KesMemoryDefault);
    auto ts = kes_malloc(dev, 8 * 4, 4, KesMemoryDefault);

    auto sem1 = kes_create_semaphore(dev, 0);
    auto sem2 = kes_create_semaphore(dev, 0);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);
    printf("y: %p (%p) (%llu bytes)\n", y.cpu, y.gpu, y.size);

    auto dma = kes_create_queue(dev, KesQueueTypeTransfer);
    auto compute = kes_create_queue(dev, KesQueueTypeCompute);

    auto l1 = kes_start_recording(dma);
    {
        kes_cmd_write_timestamp(l1, ts.gpu + 0);
        kes_cmd_memset(l1, x.gpu, size, 1);
        kes_cmd_write_timestamp(l1, ts.gpu + 8);
        kes_cmd_signal_after(l1, KesStageTransfer, y.gpu, 1337, KesSignalAtomicMax);
    }

    auto l2 = kes_start_recording(compute);
    {
        kes_cmd_write_timestamp(l2, ts.gpu + 16);
        kes_cmd_wait_before(l2, KesStageTransfer, y.gpu, 1337, KesOpEqual, KesHazardFlagsNone, ~0);
        kes_cmd_write_timestamp(l2, ts.gpu + 24);
    }

    kes_submit(dma, l1, sem1, 1);
    kes_submit(compute, l2, sem2, 1);

    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    // in this case, synch should ensure that t0 <= t1 <= t3, t2 <= t3

    printf("\n");
    printf("ts0: %lu\n", ((uint64_t *)ts.cpu)[0]);
    printf("ts1: %lu\n", ((uint64_t *)ts.cpu)[1]);
    printf("ts2: %lu\n", ((uint64_t *)ts.cpu)[2]);
    printf("ts3: %lu\n", ((uint64_t *)ts.cpu)[3]);

    kes_free(dev, &x);
    kes_destroy(dev);

    return 0;
}
