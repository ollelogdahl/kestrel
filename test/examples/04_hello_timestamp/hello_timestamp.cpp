#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

int main(void) {

    auto dev = kes_create();

    std::size_t size = 10 * 1024 * 1024;
    auto x = kes_malloc(dev, size, 4, KesMemoryDefault);
    auto y = kes_malloc(dev, 16, 4, KesMemoryDefault);

    auto sem = kes_create_semaphore(dev, 0);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);
    printf("y: %p (%p) (%llu bytes)\n", y.cpu, y.gpu, y.size);

    auto dma = kes_create_queue(dev, KesQueueTypeTransfer);

    auto l1 = kes_start_recording(dma);
    kes_cmd_write_timestamp(l1, y.gpu);
    kes_cmd_memset(l1, x.gpu, size, 2);
    kes_cmd_write_timestamp(l1, y.gpu + 8);

    kes_submit(dma, l1, sem, 1);

    auto r = kes_wait_semaphore(sem, 1);
    if (r < 0) {
        printf("wait for semaphore failed: %d\n", r);
    }

    // @todo: how to wait on cpu for DMA transfer? TODO?
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    printf("ts0: %ul\n", ((uint64_t *)y.cpu)[0]);
    printf("ts1: %ul\n", ((uint64_t *)y.cpu)[1]);

    kes_free(dev, &x);
    kes_destroy(dev);

    return 0;
}
