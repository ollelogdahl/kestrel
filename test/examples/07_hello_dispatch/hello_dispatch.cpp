#include <unistd.h>
#include <kestrel/kestrel.h>
#include <kestrel/gir.h>

#include <stdio.h>

int main(void) {
    auto dev = kes_create();

    auto x = kes_malloc(dev, sizeof(uint32_t) * 128, 8, KesMemoryReadback);

    auto sem = kes_create_semaphore(dev, 0);

    ((uint32_t *)x.cpu)[0] = 1;
    ((uint32_t *)x.cpu)[2] = 4;
    ((uint32_t *)x.cpu)[3] = 10;
    printf("x: %p %p\n", (void *)x.cpu, (void *)x.gpu);

    auto compute = kes_create_queue(dev, KesQueueTypeCompute);

    gir::Module mod;
    mod.workgroup_size_x = 32;
    {
        gir::Builder gb(mod);
        auto rp = gb.root_ptr();
        auto p = gb.add(rp, gb.mul(gb.local_invocation_index(), gb.i32(4)));
        auto x = gb.load(p);
        auto sum = gb.add(x, gb.i32(15));
        gb.store(p, sum);
    }

    auto shader = kes_create_shader(dev, (void *)&mod);

    auto cl = kes_start_recording(compute);
    {
        kes_bind_shader(cl, shader);
        kes_cmd_dispatch(cl, x.gpu, 1, 1, 1);
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
