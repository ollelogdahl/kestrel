#pragma once

#include <cstddef>
#include <cstdint>

#include <span>

namespace vektor {


struct Version {
    const char *version;
    const char *commit_id;
};

enum class QueueType {
    Graphics,
    Compute,
    Transfer
};

enum class Stage {
    Transfer,
    Compute,
    RasterColorOut,
    PixelShader,
    VertexShader
};
enum class Signal {
    AtomicSet,
    AtomicMax,
    AtomicOr,
};
enum class Op {
    Never,
    Less,
    Equal,
};

enum HazardFlags {
    // @todo
    None = 0,
    DrawArguments = 1 << 0,
    Descriptors = 1 << 1
};

enum class Memory { Default, Gpu, Readback };

typedef uint64_t gpuptr_t;
typedef void *Device;
typedef void *Queue;
typedef void *CommandList;
typedef void *Semaphore;

struct Allocation {
    void *cpu;
    gpuptr_t gpu;
    std::size_t size;
    alignas(16) std::byte _impl[16];
};


Version version();

Device create();
void destroy(Device);

Allocation malloc(Device, std::size_t size, Memory memory = Memory::Default);
Allocation malloc(Device, std::size_t size, std::size_t align, Memory memory = Memory::Default);
void free(Device, Allocation &);

Semaphore create_semaphore(uint64_t value);

Queue create_queue(Device, QueueType);
CommandList start_recording(Queue);

void submit(Queue, CommandList, Semaphore = nullptr, uint64_t value = 0);

void memset(CommandList, gpuptr_t addr, std::size_t size, uint32_t value);

void write_timestamp(CommandList, gpuptr_t addr);

void signal_after(CommandList, Stage before, gpuptr_t ptr, uint64_t value, Signal);
void wait_before(CommandList, Stage after, gpuptr_t ptr, uint64_t value, Op, HazardFlags hazard = HazardFlags::None, uint64_t mask = ~0);
void wait_semaphore(Semaphore, uint64_t value);
void signal_semaphore(Semaphore, uint64_t value);

};
