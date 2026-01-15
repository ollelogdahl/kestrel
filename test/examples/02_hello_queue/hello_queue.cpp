#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

int main(void) {

    auto dev = kes_create();

    std::size_t size = 10 * 1024 * 1024;
    auto x = kes_malloc(dev, size, 4, KesMemoryDefault);
    auto y = kes_malloc(dev, 8, 4, KesMemoryDefault);

    auto sem = kes_create_semaphore(dev, 0);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);
    printf("y: %p (%p) (%llu bytes)\n", y.cpu, y.gpu, y.size);

    // @todo: wait for address ok?

    auto dma = kes_create_queue(dev, KesQueueTypeTransfer);

    auto l1 = kes_start_recording(dma);
    kes_cmd_memset(l1, x.gpu, size, 1);
    kes_cmd_wait_before(l1, KesStageTransfer, y.gpu, 1337, KesOpEqual, KesHazardFlagsNone, ~0);
    kes_cmd_memset(l1, x.gpu, size, 2);

    kes_submit(dma, l1, sem, 1);

    // hacky bussy-wait just to prove it works.
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    while(*((uint32_t *)x.cpu) == 0);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    *((uint32_t *)y.cpu) = 1337;

    while(*((uint32_t *)x.cpu) == 1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    kes_free(dev, &x);
    kes_destroy(dev);

    return 0;
}
