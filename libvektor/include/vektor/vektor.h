#pragma once

#include <cstddef>
#include <cstdint>

#include <span>

namespace vektor {


struct Version {
    const char *version;
    const char *commit_id;
};

typedef void *Device;
typedef void *Queue;
typedef void *CommandList;

typedef uint64_t gpuptr_t;

Version version();

Device create();
void destroy(Device);

enum class Memory { Default, Gpu, Readback };

struct Allocation {
    void *cpu;
    gpuptr_t gpu;
    std::size_t size;
    alignas(16) std::byte _impl[16];
};


Allocation malloc(Device, std::size_t size, Memory memory = Memory::Default);
Allocation malloc(Device, std::size_t size, std::size_t align, Memory memory = Memory::Default);
void free(Device, Allocation &);

Queue create_queue(Device);
CommandList start_recording(Queue);

void submit(Queue, CommandList);

};
