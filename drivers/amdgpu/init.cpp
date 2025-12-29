#include "impl.h"
#include "beta.h"

std::string amdgpu_family_str(uint32_t family) {
    switch(family) {
    case AMDGPU_FAMILY_UNKNOWN: return "unknown";
    case AMDGPU_FAMILY_SI: return "si";
    case AMDGPU_FAMILY_CI: return "ci";
    case AMDGPU_FAMILY_KV: return "kv";
    case AMDGPU_FAMILY_VI: return "vi";
    case AMDGPU_FAMILY_CZ: return "cz";
    case AMDGPU_FAMILY_AI: return "Vega10";
    case AMDGPU_FAMILY_RV: return "Raven";
    case AMDGPU_FAMILY_NV: return "Navi10";
    case AMDGPU_FAMILY_VGH: return "Van Gogh";
    case AMDGPU_FAMILY_GC_11_0_0: return "GC 11.0.0";
    case AMDGPU_FAMILY_YC: return "Yellow Carp";
    case AMDGPU_FAMILY_GC_11_0_1: return "GC 11.0.1";
    case AMDGPU_FAMILY_GC_10_3_6: return "GC 10.3.6";
    case AMDGPU_FAMILY_GC_10_3_7: return "GC 10.3.7";
    case AMDGPU_FAMILY_GC_11_5_0: return "GC 11.5.0";
    case AMDGPU_FAMILY_GC_12_0_0: return "GC 12.0.0";
    }

    return "???";
}

KesDevice amdgpu_create(int drm_fd) {
    auto dev = new DeviceImpl;
    dev->fd = drm_fd;

    for (auto i = 0; i < AMDGPU_HW_IP_NUM; ++i) {
        dev->num_queues[i] = 0;
    }

    uint32_t minor, major;
    if (amdgpu_device_initialize(dev->fd, &major, &minor, &dev->amd_handle) != 0) {
        delete dev;
        return nullptr;
    }

    log("amdgpu drm loaded: {}.{}", major, minor);
    auto &info = dev->info;
    info.gfx_level = GfxLevel::GFX10_3;

    amdgpu_gpu_info gpu_info;
    if(amdgpu_query_gpu_info(dev->amd_handle, &gpu_info) != 0) {
        panic("amdgpu_query_gpu_info failed.");
	}
    log("amdgpu family: {}", amdgpu_family_str(gpu_info.family_id).c_str());

    for (auto ip_type = 0; ip_type < AMDGPU_HW_IP_NUM; ++ip_type) {
        auto &ip = info.ip[ip_type];
        drm_amdgpu_info_hw_ip ip_info;
        amdgpu_query_hw_ip_info(dev->amd_handle, ip_type, 0, &ip_info);

        ip.num_queues = std::popcount(ip_info.available_rings);

        uint32_t num_instances;
        if (amdgpu_query_hw_ip_count(dev->amd_handle, ip_type, &num_instances) != 0) {
           ip.num_instances = num_instances;
        }
    }

    drm_amdgpu_info_hw_ip gfx_ip_info;
    amdgpu_query_hw_ip_info(dev->amd_handle, AMDGPU_HW_IP_GFX, 0, &gfx_ip_info);

    log("GFX Version: {}.{}",
            gfx_ip_info.hw_ip_version_major,
            gfx_ip_info.hw_ip_version_minor);

    log("num queues: sdma: {}, {}", info.ip[AMDGPU_HW_IP_DMA].num_queues, info.ip[AMDGPU_HW_IP_DMA].num_instances);

    if(amdgpu_query_sw_info(dev->amd_handle, amdgpu_sw_info_address32_hi, &info.address32_hi) != 0) {
        panic("andgpu_query_sw_info(amdgpu_sw_info_address32_hi) failed.");
    }

    return (KesDevice)dev;
}

void amdgpu_destroy(KesDevice pd) {
    auto *dev = reinterpret_cast<DeviceImpl *>(pd);
    if (dev) {
        amdgpu_device_deinitialize(dev->amd_handle);
        dev->amd_handle = nullptr;
        delete dev;
    }
}
