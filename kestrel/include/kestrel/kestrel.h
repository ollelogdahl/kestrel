#pragma once

/**
 * @file kestrel.h
 * @brief Core Kestrel API
 */

#include <cstdint>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * GPU pointer type.
 */
typedef uint64_t kes_gpuptr_t;
/**
 * Opaque handle to a Device.
 */
typedef void *KesDevice;
/**
 * Opaque handle to a Command Queue.
 */
typedef void *KesQueue;
/**
 * Opaque handle to a Command List.
 */
typedef void *KesCommandList;

/**
 * Structure describing a memory allocation.
 * @sa kes_malloc
 */
struct KesAllocation {
    /**
     * Pointer to the CPU-accessible memory.
     */
    void *cpu;
    /**
     * GPU pointer to the allocated memory.
     */
    kes_gpuptr_t gpu;
    /**
     * Size of the allocation in bytes.
     */
    size_t size;
    /**
     * Internal driver-specific data. Should not be modified directly.
     */
    void *_internal;
};

/**
 * Memory types for allocation.
 * @sa kes_malloc
 */
enum KesMemory {
    /**
     * Memory type suitable for most data (uniforms, etc). Always visible to both the CPU and GPU.
     */
    KesMemoryDefault,
    /**
     * Memory type optimized for GPU access, not visible to the CPU. Use for large buffers or textures
     * that the CPU does not need to read or write.
     */
    KesMemoryGpu,
    /**
     * Memory type optimized for CPU readback. Visible to both CPU and GPU, but optimized for CPU reads.
     */
    KesMemoryReadback
};

/**
 * Special flags for wait operations.
 * @sa kes_cmd_wait_before
 */
enum KesHazardFlags {
    /**
     * No special hazard flags.
     */
    KesHazardFlagsNone = 0,
    /**
     * Indicates that the wait is for draw arguments.
     */
    KesHazardFlagsDrawArguments = 1 << 0,
    /**
     * Indicates that the wait is for descriptor updates.
     */
    KesHazardFlagsDescriptors = 1 << 1,
};

/**
 * Comparison operations for wait operations.
 * @sa kes_cmd_wait_before
 */
enum KesOp {
    /**
     * Never pass the wait.
     */
    KesOpNever,
    /**
     * Pass the wait if the value is less than the specified value.
     */
    KesOpLess,
    /**
     * Pass the wait if the value is equal to the specified value.
     */
    KesOpEqual
};

/**
 * Signal operations for signal operations.
 * @sa kes_cmd_signal_after
 */
enum KesSignal {
    /**
     * Set the value atomically.
     */
    KesSignalAtomicSet,
    /**
     * Set the value to the maximum of the current and specified value atomically.
     */
    KesSignalAtomicMax,
    /**
     * Perform a bitwise OR with the specified value atomically.
     */
    KesSignalAtomicOr,
};

/**
 * Pipeline stages for synchronization.
 * @sa kes_cmd_signal_after, kes_cmd_wait_before
 */
enum KesStage {
    /**
     * Transfer stage. Should only be used on transfer queues.
     */
    KesStageTransfer,
    /**
     * Compute stage.
     */
    KesStageCompute,
    /**
     * Raster color output stage (e.g., render target writes).
     */
    KesStageRasterColorOut,
    /**
     * Pixel shader stage.
     */
    KesStagePixelShader,
    /**
     * Vertex shader stage.
     */
    KesStageVertexShader
};

/**
 * Types of command queues.
 * @sa kes_create_queue
 */
enum KesQueueType {
    /**
     * Graphics queue, capable of rasterization and compute.
     */
    KesQueueTypeGraphics,
    /**
     * Compute-only queue.
     */
    KesQueueTypeCompute,
    /**
     * Transfer-only queue, optimized for memory copy and fill operations.
     */
    KesQueueTypeTransfer
};

/**
 * Create a new Device.
 * @return A handle to the created device.
 */
KesDevice kes_create();

/**
 * Destroy a Device.
 * @param device The device to destroy.
 */
void kes_destroy(KesDevice device);

/**
 * Allocate memory on the device.
 * @param device The device to allocate memory on.
 * @param size The size of the allocation in bytes.
 * @param align The alignment of the allocation in bytes.
 * @param memory The type of memory to allocate.
 * @return A KesAllocation structure describing the allocated memory.
 */
struct KesAllocation kes_malloc(KesDevice device, size_t size, size_t align, enum KesMemory memory);

/**
 * Free a previously allocated memory allocation.
 * @param device The device the memory was allocated on.
 * @param allocation The allocation to free.
 */
void kes_free(KesDevice device, struct KesAllocation *allocation);

/**
 * Create a command queue on the device.
 * @param device The device to create the queue on.
 * @param type The type of queue to create.
 * @return A handle to the created queue.
 */
KesQueue kes_create_queue(KesDevice device, enum KesQueueType type);

/**
 * Destroy a command queue.
 * @param queue The queue to destroy.
 */
void kes_destroy_queue(KesQueue queue);

/**
 * Start recording commands on a command queue.
 * @param queue The queue to record commands on.
 * @return A handle to the command list for recording commands.
 */
KesCommandList kes_start_recording(KesQueue queue);

/**
 * Submit a recorded command list to a command queue for execution.
 * @param queue The queue to submit the command list to. Must be the queue that the command list was created for.
 * @param command_list The command list to submit.
 */
void kes_submit(KesQueue queue, KesCommandList command_list);

/**
 * Record a memory set command in the command list.
 * @param command_list The command list to record the command in.
 * @param addr The GPU address to set memory at.
 * @param size The size of the memory to set in bytes.
 * @param value The value to set each byte to.
 */
void kes_cmd_memset(KesCommandList command_list, kes_gpuptr_t addr, size_t size, uint32_t value);

/**
 * Record a timestamp write command in the command list. Writes a 64-bit unsigned timestamp in ticks to the provided address.
 * @param command_list The command list to record the command in.
 * @param addr The GPU address to write the timestamp to.
 */
void kes_cmd_write_timestamp(KesCommandList command_list, kes_gpuptr_t addr);

/**
 * Record a signal command in the command list.
 * @param command_list The command list to record the command in.
 * @param before The pipeline stage before which the signal should occur.
 * @param addr The GPU address to signal to.
 * @param value The value to signal.
 * @param signal The type of signal operation to perform.
 */
void kes_cmd_signal_after(KesCommandList command_list, enum KesStage before, kes_gpuptr_t addr, uint64_t value, enum KesSignal signal);

/**
 * Record a wait command in the command list.
 * @param command_list The command list to record the command in.
 * @param after The pipeline stage after which the wait should occur.
 * @param addr The GPU address to wait on.
 * @param value The value to compare against.
 * @param op The comparison operation to use.
 * @param hazard_flags Special hazard flags for the wait operation.
 * @param mask A bitmask to apply to the value before comparison.
 */
void kes_cmd_wait_before(KesCommandList command_list, enum KesStage after, kes_gpuptr_t addr, uint64_t value, enum KesOp op, enum KesHazardFlags hazard_flags, uint64_t mask);

#ifdef __cplusplus
}
#endif
