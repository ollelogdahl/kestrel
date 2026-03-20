#include <cstdlib>
#include <unistd.h>
#include <kestrel/kestrel.h>

#include <stdio.h>

int main(void) {
    auto dev = kes_create();

    int n = 10;
    for (auto i = 0; i < n; ++i) {
        auto size = rand() % 32;
        auto align = 4;

        auto x = kes_malloc(dev, size, align, KesMemoryDefault);
        printf("cpu: %p gpu: %p sz: %d\n", (void *)x.cpu, (void *)x.gpu, x.size);
    }

    return 0;
}
