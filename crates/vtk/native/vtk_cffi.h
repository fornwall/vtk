#ifndef VTK_CFFI_INCLUDED
#define VTK_CFFI_INCLUDED

#ifdef VTK_CUSTOM_VULKAN_TYPES
#include "vtk_cffi_vulkan.h"
#else
#include "vulkan_wrapper.h"
#endif

struct VtkApplicationNative {
    /** Null-terminated, static string. <div rustbindgen private> */
    void* platform_specific;
};

struct VtkDeviceNative {
    _Bool initialized;
    VkInstance vk_instance;
    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    uint32_t queue_family_index;
    VkQueue vk_queue;
};

struct VtkWindowNative {
    struct VtkDeviceNative* vtk_device;
    uint32_t width;
    uint32_t height;
    VkSurfaceKHR vk_surface;
    VkSwapchainKHR vk_swapchain;
    /** Platform-specific data. <div rustbindgen private> */
    void* platform_specific;
};

/** Null-terminated, static string. <div rustbindgen private> */
VkShaderModule vtk_compile_shader(VkDevice vk_device, uint8_t const* bytes, size_t size);

struct VtkApplicationNative* vtk_application_init();

// Run the event loop.
void vtk_application_run(struct VtkApplicationNative* application);

struct VtkDeviceNative* vtk_device_init(struct VtkApplicationNative* vtk_application);

struct VtkWindowNative* vtk_window_init(struct VtkDeviceNative* vtk_device);

#endif