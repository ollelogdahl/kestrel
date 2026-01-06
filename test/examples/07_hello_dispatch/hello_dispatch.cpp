#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

struct DispatchArguments {
    uint64_t buffer;
};

int main(void) {
    auto dev = kes_create();

    auto x = kes_malloc(dev, 1024, 4, KesMemoryDefault);
    auto y = kes_malloc(dev, sizeof(DispatchArguments), 8, KesMemoryDefault);

    printf("x: %p %p\n", (void *)x.cpu, (void *)x.gpu);
    printf("y: %p %p\n", (void *)y.cpu, (void *)y.gpu);

    DispatchArguments *args = (DispatchArguments *)y.cpu;
    args->buffer = x.gpu;

    auto compute = kes_create_queue(dev, KesQueueTypeCompute);

    auto cl = kes_start_recording(compute);
    {
        kes_cmd_dispatch(cl, y.gpu, 32, 1, 1);
    }

    kes_submit(compute, cl);

    sleep(1);

    printf("x[0]: %u\n", ((uint32_t *)x.cpu)[0]);
    printf("x[1]: %u\n", ((uint32_t *)x.cpu)[1]);
    printf("x[2]: %u\n", ((uint32_t *)x.cpu)[2]);
    printf("x[3]: %u\n", ((uint32_t *)x.cpu)[3]);

    kes_free(dev, &x);
    kes_destroy(dev);

    return 0;
}
