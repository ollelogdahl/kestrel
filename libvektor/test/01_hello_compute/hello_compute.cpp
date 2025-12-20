#include <unistd.h>
#include <vektor/vektor.h>

#include <stdio.h>

int main(void) {

    auto version = vektor::version();
    printf("vektor %s (%s)\n", version.version, version.commit_id);

    auto dev = vektor::create();

    auto x = vektor::malloc(dev, 100 * 1024 * 1024);

    printf("x: %p (%p) (%llu bytes)\n", x.cpu, x.gpu, x.size);

    vektor::free(dev, x);

    vektor::destroy(dev);

    return 0;
}
