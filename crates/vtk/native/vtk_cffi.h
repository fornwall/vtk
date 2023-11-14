#ifdef VTK_INCLUDE_VULKAN_PROPER
#include "vulkan_wrapper.h"
#include <stdbool.h>
#else
#include "vtk_vulkan.h"
#endif

struct VtkApplication {
    /** Null-terminated, static string. <div rustbindgen private> */
    void* platform_app;
};

struct VtkDevice {
    _Bool initialized;
    VkInstance vk_instance;
    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    uint32_t queue_family_index;
    VkQueue vk_queue;
};

struct VtkWindow {
    uint32_t width;
    uint32_t height;
    VkSurfaceKHR vk_surface;
    VkSwapchainKHR vk_swapchain;
};

/** Null-terminated, static string. <div rustbindgen private> */
VkShaderModule compile_shader(VkDevice device, uint8_t const* bytes, size_t size);

_Bool vtk_application_init(struct VtkApplication* application);
_Bool vtk_device_init(struct VtkDevice* device);
