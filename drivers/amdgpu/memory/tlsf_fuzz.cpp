#include <cstdint>
#include <cstddef>
#include <vector>
#include "tlsf.h"

enum class OpType : uint8_t {
    Allocate = 0,
    Free = 1,
    CheckStats = 2,
    Count
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 8) return 0;

    uint64_t heapSize = *reinterpret_cast<const uint64_t*>(data);
    heapSize = 256 + (heapSize % (16 * 1024 * 1024));

    data += 8;
    size -= 8;

    TlsfAllocator allocator(heapSize);

    std::vector<TlsfAllocator::Allocation> activeAllocations;
    activeAllocations.reserve(1024);

    size_t offset = 0;
    while (offset < size) {
        OpType op = static_cast<OpType>(data[offset++] % static_cast<uint8_t>(OpType::Count));

        if (op == OpType::Allocate) {
            if (size - offset < 16) break;

            uint64_t allocSize;
            std::memcpy(&allocSize, data + offset, sizeof(allocSize));
            offset += 8;
            uint64_t rawAlignment;
            std::memcpy(&rawAlignment, data + offset, sizeof(rawAlignment));
            offset += 8;

            allocSize = 1 + (allocSize % (heapSize * 2));

            uint64_t alignment = 1ULL << (rawAlignment % 13); // 2^0 up to 2^12 (4096)

            TlsfAllocator::Allocation alloc;
            bool success = allocator.allocate(allocSize, alignment, alloc);

            if (success) {
                // Ensure opaque pointers are properly returned and tracking guarantees stick
                if (alloc._block == nullptr || alloc.size < allocSize) {
                    __builtin_trap();
                }
                activeAllocations.push_back(alloc);
            }

        } else if (op == OpType::Free) {
            if (activeAllocations.empty()) continue;

            if (size - offset < 2) break;
            uint16_t indexChoice;
            std::memcpy(&indexChoice, data + offset, sizeof(indexChoice));
            offset += 2;

            size_t targetIndex = indexChoice % activeAllocations.size();

            allocator.free(activeAllocations[targetIndex]);

            if (targetIndex != activeAllocations.size() - 1) {
                activeAllocations[targetIndex] = activeAllocations.back();
            }
            activeAllocations.pop_back();

        } else if (op == OpType::CheckStats) {
            TlsfAllocator::Stats s = allocator.stats();

            uint64_t calculatedTotal = s.freeBytes + s.usedBytes + s.nullBlockSize;
            if (calculatedTotal != heapSize) {
                __builtin_trap();
            }
        }
    }

    for (const auto& alloc : activeAllocations) {
        allocator.free(alloc);
    }

    return 0;
}
