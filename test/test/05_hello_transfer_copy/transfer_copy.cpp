#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

int main(void) {

    auto dev = kes_create();

    std::size_t size = 10 * 1024 * 1024;
    auto x = kes_malloc(dev, size, 4, KesMemoryDefault);
    auto y = kes_malloc(dev, size, 4, KesMemoryDefault);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);
    printf("y: %p (%p) (%llu bytes)\n", y.cpu, y.gpu, y.size);

    auto dma = kes_create_queue(dev, KesQueueTypeTransfer);

    sleep(1);

    auto l1 = kes_start_recording(dma);
    kes_cmd_memset(l1, x.gpu, size, 2);
    kes_cmd_memcpy(l1, y.gpu, x.gpu, size);
    kes_submit(dma, l1);

    // @todo: hacky bussy-wait
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    printf("y[0]: %u\n", ((uint32_t *)y.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    printf("y[0]: %u\n", ((uint32_t *)y.cpu)[0]);

    kes_free(dev, &x);
    kes_destroy(dev);

    return 0;
}
