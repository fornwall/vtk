#include <stdint.h>
#include <stdlib.h>

#include "vtk_array.h"
#include "vtk_cffi.h"
#include "vtk_log.h"
#include "vtk_internal.h"
#include "vulkan_wrapper.h"

struct VtkDeviceNative* vtk_device_init(struct VtkContextNative* vtk_context)
{
    struct VtkDeviceNative* device = (struct VtkDeviceNative*) malloc(sizeof(struct VtkDeviceNative));
    device->vtk_context = vtk_context;

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "vulkan-example",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "vulkan-example",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_MAKE_VERSION(1, 2, 0),
    };

    const char *instance_extensions[] = {
        "VK_KHR_surface",
#ifdef __ANDROID__
        "VK_KHR_android_surface",
#elif defined __APPLE__
	    "VK_EXT_metal_surface",
#ifndef VTK_NO_VULKAN_LOADING
	    "VK_KHR_portability_enumeration",
	    "VK_KHR_get_physical_device_properties2",
#endif
#else
        "VK_KHR_wayland_surface",
#endif
    };

    char const *enabledLayerNames[] = {
#ifdef VTK_VULKAN_VALIDATION
         "VK_LAYER_KHRONOS_validation"
#endif
    };

    VkInstanceCreateInfo instance_create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = NULL,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = VTK_ARRAY_SIZE(enabledLayerNames),
            .ppEnabledLayerNames = enabledLayerNames,
            .enabledExtensionCount = VTK_ARRAY_SIZE(instance_extensions),
            .ppEnabledExtensionNames = instance_extensions,
#if defined(__APPLE__) && !defined(VTK_NO_VULKAN_LOADING)
            // Necessary to load MoltenVK through the vulkan loader:
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
    };


    assert(vkCreateInstance != NULL);
    LOGI("Before vkCreateInstance");
    CALL_VK(vkCreateInstance(&instance_create_info, NULL, &device->vk_instance));
    LOGI("AFter vkCreateInstance");

    uint32_t gpu_count = 0;
    CALL_VK(vkEnumeratePhysicalDevices(device->vk_instance, &gpu_count, NULL));

    LOGI("Number of gpus: %d", gpu_count);
    VkPhysicalDevice gpus[gpu_count];
    CALL_VK(vkEnumeratePhysicalDevices(device->vk_instance, &gpu_count, gpus));
    for (int i = 0; i < gpu_count; i++) {
        VkPhysicalDevice gpu = gpus[i];
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(gpu, &properties);
        switch (properties.deviceType) {
        default :
            continue;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER :
            LOGI("gpu %d: Type other", i);
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU :
            LOGI("gpu %d: Type integrated", i);
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU :
            LOGI("gpu %d: Type discrete", i);
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU :
            LOGI("gpu %d: Type virtual", i);
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU :
            LOGI("gpu %d: Type cpu", i);
            break;
        }
    }

    // Select the first device:
    device->vk_physical_device = gpus[0];

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device->vk_physical_device, &queueFamilyCount, NULL);
    LOGI("Device queue family count: %d", queueFamilyCount);
    assert(queueFamilyCount);
    VkQueueFamilyProperties* queueFamilyProperties = VTK_ARRAY_ALLOC(VkQueueFamilyProperties, queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device->vk_physical_device, &queueFamilyCount, queueFamilyProperties);
    uint32_t queue_family_idx;
    /*
    for (queue_family_idx = 0; queue_family_idx < queueFamilyCount; queue_family_idx++) {
        uint32_t flags = queueFamilyProperties[queue_family_idx].queueFlags;
        LOGI("Queue family: %d", queue_family_idx);
        if (flags & VK_QUEUE_GRAPHICS_BIT) LOGI("  - VK_QUEUE_GRAPHICS_BIT");
        if (flags & VK_QUEUE_COMPUTE_BIT) LOGI("  - VK_QUEUE_COMPUTE_BIT");
        if (flags & VK_QUEUE_TRANSFER_BIT) LOGI("  - VK_QUEUE_TRANSFER_BIT");
    }
    */
    for (queue_family_idx = 0; queue_family_idx < queueFamilyCount; queue_family_idx++) {
        if (queueFamilyProperties[queue_family_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    free(queueFamilyProperties);
    assert(queue_family_idx < queueFamilyCount);
    device->graphics_queue_family_idx = queue_family_idx;

    float priorities[] = {1.0f};

    VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = queue_family_idx,
            .queueCount = 1,
            .pQueuePriorities = priorities,
    };

    char const* device_extensions[] = {
       "VK_KHR_swapchain",
#ifdef __APPLE__
	    "VK_KHR_portability_subset",
#endif
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = VTK_ARRAY_SIZE(device_extensions),
        .ppEnabledExtensionNames = device_extensions,
        .pEnabledFeatures = NULL,
    };

    CALL_VK(vkCreateDevice(device->vk_physical_device, &deviceCreateInfo, NULL, &device->vk_device));
    vkGetDeviceQueue(device->vk_device, device->graphics_queue_family_idx, 0, &device->vk_queue);

    VkCommandPoolCreateInfo vk_command_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            // There are two possible flags for command pools:
            // - VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded
            //    with new commands very often (may change memory allocation behavior).
            // - VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be
            //   rerecorded individually, without this flag they all have to be reset together.
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = device->graphics_queue_family_idx,
    };
    CALL_VK(vkCreateCommandPool(device->vk_device, &vk_command_pool_create_info, NULL, &device->vk_command_pool));
    // TODO: Cleanup with VkDestroyCommandPool

    return device;
}

__attribute__ ((visibility ("default")))
struct VtkWindowNative* vtk_window_init(struct VtkDeviceNative* vtk_device) {
   struct VtkWindowNative* vtk_window = (struct VtkWindowNative*) malloc(sizeof(struct VtkWindowNative));
   vtk_window->vtk_device = vtk_device;
   vtk_window_init_platform(vtk_window);
   return vtk_window;
}

VkShaderModule vtk_device_create_shader(struct VtkDeviceNative* vtk_device, uint8_t const* bytes, size_t size) {
    VkDevice vk_device = vtk_device->vk_device;
    VkShaderModuleCreateInfo vk_shader_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .codeSize = size,
            .pCode = (const uint32_t *) bytes,
    };
    VkShaderModule result;
    CALL_VK(vkCreateShaderModule(vk_device, &vk_shader_module_create_info, NULL, &result));
    return result;
}
