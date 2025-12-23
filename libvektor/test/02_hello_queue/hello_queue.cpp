#include <unistd.h>
#include <vektor/vektor.h>

#include <stdio.h>

int main(void) {

    auto version = vektor::version();
    printf("vektor %s (%s)\n", version.version, version.commit_id);

    auto dev = vektor::create();

    std::size_t size = 10 * 1024 * 1024;
    auto x = vektor::malloc(dev, size);
    auto y = vektor::malloc(dev, 8);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);
    printf("y: %p (%p) (%llu bytes)\n", y.cpu, y.gpu, y.size);

    // @todo: wait for address ok?

    auto dma = vektor::create_queue(dev, vektor::QueueType::Transfer);

    auto l1 = vektor::start_recording(dma);
    vektor::memset(l1, x.gpu, size, 1);
    vektor::wait_before(l1, vektor::Stage::Transfer, y.gpu, 1337, vektor::Op::Equal);
    vektor::memset(l1, x.gpu, size, 2);

    vektor::submit(dma, l1);

    // @todo: hacky bussy-wait
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    while(*((uint32_t *)x.cpu) == 0);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    *((uint32_t *)y.cpu) = 1337;

    while(*((uint32_t *)x.cpu) == 1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    vektor::free(dev, x);
    vektor::destroy(dev);

    return 0;
}
