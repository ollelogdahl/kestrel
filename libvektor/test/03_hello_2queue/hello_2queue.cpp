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
    auto ts = vektor::malloc(dev, 8 * 5);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);
    printf("y: %p (%p) (%llu bytes)\n", y.cpu, y.gpu, y.size);

    auto dma1 = vektor::create_queue(dev, vektor::QueueType::Transfer);
    auto dma2 = vektor::create_queue(dev, vektor::QueueType::Transfer);

    auto l1 = vektor::start_recording(dma1);
    {
        vektor::write_timestamp(l1, ts.gpu + 0);
        vektor::memset(l1, x.gpu, size, 1);
        vektor::write_timestamp(l1, ts.gpu + 8);
        vektor::signal_after(l1, vektor::Stage::Transfer, y.gpu, 1337, vektor::Signal::AtomicMax);
    }

    auto l2 = vektor::start_recording(dma2);
    {
        vektor::write_timestamp(l2, ts.gpu + 16);
        vektor::wait_before(l2, vektor::Stage::Transfer, y.gpu, 1337, vektor::Op::Equal);
        vektor::write_timestamp(l2, ts.gpu + 24);
        vektor::memset(l2, x.gpu, size, 2);
        vektor::write_timestamp(l2, ts.gpu + 32);
    }

    vektor::submit(dma2, l2);
    vektor::submit(dma1, l1);

    // @todo: how to wait on cpu for DMA transfer? TODO?
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    sleep(1);
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);

    printf("\n");
    printf("ts0: %lu\n", ((uint64_t *)ts.cpu)[0]);
    printf("ts1: %lu\n", ((uint64_t *)ts.cpu)[1]);
    printf("ts2: %lu\n", ((uint64_t *)ts.cpu)[2]);
    printf("ts3: %lu\n", ((uint64_t *)ts.cpu)[3]);
    printf("ts4: %lu\n", ((uint64_t *)ts.cpu)[4]);

    vektor::free(dev, x);
    vektor::destroy(dev);

    return 0;
}
