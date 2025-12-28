#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

int main(void) {

    auto dev = kes_create();

    std::size_t size = 10 * 1024 * 1024;
    auto x = kes_malloc(dev, size, 4, KesMemoryDefault);
    auto y = kes_malloc(dev, 8, 4, KesMemoryDefault);
    auto ts = kes_malloc(dev, 8 * 5, 4, KesMemoryDefault);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);
    printf("y: %p (%p) (%llu bytes)\n", y.cpu, y.gpu, y.size);

    auto dma1 = kes_create_queue(dev, KesQueueTypeTransfer);
    auto dma2 = kes_create_queue(dev, KesQueueTypeTransfer);

    auto l1 = kes_start_recording(dma1);
    {
        kes_cmd_write_timestamp(l1, ts.gpu + 0);
        kes_cmd_memset(l1, x.gpu, size, 1);
        kes_cmd_write_timestamp(l1, ts.gpu + 8);
        kes_cmd_signal_after(l1, KesStageTransfer, y.gpu, 1337, KesSignalAtomicMax);
    }

    auto l2 = kes_start_recording(dma2);
    {
        kes_cmd_write_timestamp(l2, ts.gpu + 16);
        kes_cmd_wait_before(l2, KesStageTransfer, y.gpu, 1337, KesOpEqual, KesHazardFlagsNone, ~0);
        kes_cmd_write_timestamp(l2, ts.gpu + 24);
        kes_cmd_memset(l2, x.gpu, size, 2);
        kes_cmd_write_timestamp(l2, ts.gpu + 32);
    }

    kes_submit(dma2, l2);
    kes_submit(dma1, l1);

    // @todo: how to wait on cpu for DMA transfer? TODO?
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    printf("\n");
    printf("ts0: %lu\n", ((uint64_t *)ts.cpu)[0]);
    printf("ts1: %lu\n", ((uint64_t *)ts.cpu)[1]);
    printf("ts2: %lu\n", ((uint64_t *)ts.cpu)[2]);
    printf("ts3: %lu\n", ((uint64_t *)ts.cpu)[3]);
    printf("ts4: %lu\n", ((uint64_t *)ts.cpu)[4]);

    kes_free(dev, &x);
    kes_destroy(dev);

    return 0;
}
