#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

struct DispatchArguments {
    uint64_t va;
    uint32_t size;
};

int main(void) {

    auto size = 10 * 1024 * 1024;

    auto dev = kes_create();

    auto x = kes_malloc(dev, size, 4, KesMemoryDefault);
    auto y = kes_malloc(dev, sizeof(DispatchArguments), 8, KesMemoryDefault);

    DispatchArguments *args = (DispatchArguments *)y.cpu;
    args->va = x.gpu;
    args->size = size;

    auto compute = kes_create_queue(dev, KesQueueTypeCompute);

    auto cl = kes_start_recording(compute);
    {
        kes_cmd_dispatch(cl, y.gpu, 128, 1, 1);
    }

    kes_submit(compute, cl);

    sleep(1);

    kes_free(dev, &x);
    kes_destroy(dev);

    return 0;
}
