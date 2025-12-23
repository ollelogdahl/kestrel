#include <unistd.h>
#include <vektor/vektor.h>

#include <stdio.h>

int main(void) {

    auto version = vektor::version();
    printf("vektor %s (%s)\n", version.version, version.commit_id);

    auto dev = vektor::create();

    std::size_t size = 10 * 1024 * 1024;
    auto x = vektor::malloc(dev, size);
    auto y = vektor::malloc(dev, 16);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);
    printf("y: %p (%p) (%llu bytes)\n", y.cpu, y.gpu, y.size);

    auto dma = vektor::create_queue(dev, vektor::QueueType::Transfer);

    auto l1 = vektor::start_recording(dma);
    vektor::write_timestamp(l1, y.gpu);
    vektor::memset(l1, x.gpu, size, 2);
    vektor::write_timestamp(l1, y.gpu + 8);

    vektor::submit(dma, l1);

    // @todo: how to wait on cpu for DMA transfer? TODO?
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    printf("ts0: %ul\n", ((uint64_t *)y.cpu)[0]);
    printf("ts1: %ul\n", ((uint64_t *)y.cpu)[1]);

    vektor::free(dev, x);
    vektor::destroy(dev);

    return 0;
}
