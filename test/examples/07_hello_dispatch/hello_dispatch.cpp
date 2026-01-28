#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

int main(void) {
    auto dev = kes_create();

    auto x = kes_malloc(dev, sizeof(uint32_t) * 128, 8, KesMemoryReadback);

    auto sem = kes_create_semaphore(dev, 0);

    printf("x: %p %p\n", (void *)x.cpu, (void *)x.gpu);

    auto compute = kes_create_queue(dev, KesQueueTypeCompute);

    auto cl = kes_start_recording(compute);
    {
        kes_cmd_dispatch(cl, x.gpu, 32, 1, 1);
    }

    kes_submit(compute, cl, sem, 1);

    auto r = kes_wait_semaphore(sem, 1);
    if (r < 0) {
        printf("wait for semaphore failed: %d\n", r);
    }
    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    printf("x[1]: %u\n", ((uint32_t *)x.cpu)[1]);
    printf("x[2]: %u\n", ((uint32_t *)x.cpu)[2]);
    printf("x[3]: %u\n", ((uint32_t *)x.cpu)[3]);

    sleep(1);

    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    printf("x[1]: %u\n", ((uint32_t *)x.cpu)[1]);
    printf("x[2]: %u\n", ((uint32_t *)x.cpu)[2]);
    printf("x[3]: %u\n", ((uint32_t *)x.cpu)[3]);

    return 0;
}
