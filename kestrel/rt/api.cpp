#include "kestrel/kestrel.h"
#include "kestrel/interface.h"
#include <fcntl.h>

#define API_EXPORT __attribute__((visibility("default")))

#include <vector>
#include <filesystem>
#include <string>
#include <fstream>

#include <unistd.h>
#include <dlfcn.h>

struct DiscoveryInfo {
    std::string device_node;
    std::string driver_name;
    uint32_t vendor_id;
};

std::vector<DiscoveryInfo> discover_gpus() {
    std::vector<DiscoveryInfo> found;
    std::string base_path = "/sys/class/drm";

    for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
        std::string name = entry.path().filename().string();

        if (name.compare(0, 7, "renderD") == 0) {
            DiscoveryInfo info;
            info.device_node = "/dev/dri/" + name;

            std::filesystem::path driver_path = entry.path() / "device/driver";
            if (std::filesystem::exists(driver_path)) {
                info.driver_name = std::filesystem::read_symlink(driver_path).filename().string();
            }

            std::ifstream vendor_file(entry.path() / "device/vendor");
            vendor_file >> std::hex >> info.vendor_id;

            found.push_back(info);
        }
    }
    return found;
}

struct DeviceHandle {
    int fd;
    KesDevice drv_handle;
    KesDriverFuncs fns;
};

struct QueueHandle {
    DeviceHandle *dev;
    KesQueue queue;
};

struct CommandListHandle {
    DeviceHandle *dev;
    KesCommandList cmdlist;
};

API_EXPORT KesDevice kes_create() {
    auto gpus = discover_gpus();

    for (const auto& gpu : gpus) {
        printf("GPU: %s (%s)\n", gpu.device_node.c_str(), gpu.driver_name.c_str());

        std::string lib_name = "libkes_" + gpu.driver_name + ".so";

        std::string temp_path = std::string("/home/olle/hack/kestrel/build-dev/drivers/") + lib_name;

        printf("trying path: %s\n", temp_path.c_str());

        void* handle = dlopen(temp_path.c_str(), RTLD_NOW);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            continue;
        }

        auto drv_interface = (typeof(kes_drv_interface)*)dlsym(handle, "kes_drv_interface");
        if (!drv_interface) {
            fprintf(stderr, "dlsym failed: %s\n", dlerror());
            dlclose(handle);
            continue;
        }

        auto* dev = new DeviceHandle{};
        drv_interface(&dev->fns);

        // Open the node once we know we have the right driver
        int fd = open(gpu.device_node.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0) continue;

        dev->fd = fd;
        dev->drv_handle = dev->fns.fn_create(fd);
        printf("drv handle: %p\n", dev->drv_handle);

        if (!dev->drv_handle) {
            close(fd);
            dlclose(handle);
            delete dev;
        }

        return dev;
    }

    return nullptr;
}

API_EXPORT void kes_destroy(KesDevice pd) {
    auto *dev = reinterpret_cast<DeviceHandle *>(pd);
    if (dev) {
        dev->fns.fn_destroy(dev->drv_handle);
        dev->drv_handle = nullptr;
        close(dev->fd);
        delete dev;
    }
}

API_EXPORT KesAllocation kes_malloc(KesDevice pd, size_t size, size_t align, KesMemory memory) {
    auto *dev = reinterpret_cast<DeviceHandle *>(pd);
    return dev->fns.fn_malloc(dev->drv_handle, size, align, memory);
}

API_EXPORT void kes_free(KesDevice pd, struct KesAllocation *alloc) {
    auto *dev = reinterpret_cast<DeviceHandle *>(pd);
    dev->fns.fn_free(dev->drv_handle, alloc);
}

API_EXPORT KesQueue kes_create_queue(KesDevice pd, KesQueueType type) {
    auto *dev = reinterpret_cast<DeviceHandle *>(pd);
    auto queue = dev->fns.fn_create_queue(dev->drv_handle, type);

    auto *qhandle = new QueueHandle{};
    qhandle->dev = dev;
    qhandle->queue = queue;
    return qhandle;
}

API_EXPORT void kes_destroy_queue(KesQueue pq) {
    auto *qhandle = reinterpret_cast<QueueHandle *>(pq);
    if (qhandle) {
        auto *dev = qhandle->dev;
        dev->fns.fn_destroy_queue(qhandle->queue);
        delete qhandle;
    }
}

API_EXPORT KesCommandList kes_start_recording(KesQueue pq) {
    auto *qhandle = reinterpret_cast<QueueHandle *>(pq);
    auto *dev = qhandle->dev;
    auto *clhandle = new CommandListHandle{};
    clhandle->dev = qhandle->dev;
    clhandle->cmdlist = dev->fns.fn_start_recording(qhandle->queue);

    return clhandle;
}

API_EXPORT void kes_submit(KesQueue pq, KesCommandList pcl) {
    auto *qhandle = reinterpret_cast<QueueHandle *>(pq);
    auto *clhandle = reinterpret_cast<CommandListHandle *>(pcl);
    auto *dev = qhandle->dev;

    dev->fns.fn_submit(qhandle->queue, clhandle->cmdlist);

    delete clhandle;
}

API_EXPORT void kes_cmd_memset(KesCommandList pcl, kes_gpuptr_t addr, size_t size, uint32_t value) {
    auto *clhandle = reinterpret_cast<CommandListHandle *>(pcl);
    auto *dev = clhandle->dev;

    dev->fns.fn_cmd_memset(clhandle->cmdlist, addr, size, value);
}
API_EXPORT void kes_cmd_write_timestamp(KesCommandList pcl, kes_gpuptr_t addr) {
    auto *clhandle = reinterpret_cast<CommandListHandle *>(pcl);
    auto *dev = clhandle->dev;

    dev->fns.fn_cmd_write_timestamp(clhandle->cmdlist, addr);
}

API_EXPORT void kes_cmd_signal_after(KesCommandList pcl, KesStage before, kes_gpuptr_t addr, uint64_t value, KesSignal signal) {
    auto *clhandle = reinterpret_cast<CommandListHandle *>(pcl);
    auto *dev = clhandle->dev;

    dev->fns.fn_cmd_signal_after(clhandle->cmdlist, before, addr, value, signal);
}

API_EXPORT void kes_cmd_wait_before(KesCommandList pcl, KesStage after, kes_gpuptr_t addr, uint64_t value, KesOp op, KesHazardFlags hazard, uint64_t mask) {
    auto *clhandle = reinterpret_cast<CommandListHandle *>(pcl);
    auto *dev = clhandle->dev;

    dev->fns.fn_cmd_wait_before(clhandle->cmdlist, after, addr, value, op, hazard, mask);
}
