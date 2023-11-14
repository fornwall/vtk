#ifdef VTK_INCLUDE_VULKAN_PROPER
#include "vulkan_wrapper.h"
#include <stdbool.h>
#else
#include "vtk_vulkan.h"
#endif

struct VulkanDevice {
    _Bool initialized;
    VkInstance vk_instance;
    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    uint32_t queue_family_index;
    VkQueue vk_queue;
};

VkShaderModule compile_shader(VkDevice device, uint8_t const* bytes, size_t size);

_Bool init_vulkan_device(struct VulkanDevice* device);

struct VulkanWindow {
    VkSurfaceKHR vk_surface;
};
