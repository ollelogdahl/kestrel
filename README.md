# Kestrel

Work-in-Progress GPGPU API and driver. Currently only developing for RDNA 2.

[Documentation](https://ollelogdahl.github.io/kestrel/)

## Building

```sh
# build everything
cmake -B build
cmake --build build

# build docs
cmake --build build -t docs
```

Example programs exist in `test/examples` and can be ran like `./build/test/example_01_hello_malloc`.
