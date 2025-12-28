#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

int main(void) {

    auto dev = kes_create();

    auto x = kes_malloc(dev, 100 * 1024 * 1024, 4, KesMemoryDefault);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);

    kes_free(dev, &x);

    kes_destroy(dev);

    return 0;
}
