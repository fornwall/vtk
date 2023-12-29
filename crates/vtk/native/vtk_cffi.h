#ifndef VTK_CFFI_INCLUDED
#define VTK_CFFI_INCLUDED

#ifdef VTK_RUST_BINDGEN
#include "vtk_cffi_vulkan.h"
typedef void NSApplication;
typedef void VtkViewController;
#else // VTK_RUST_BINDGEN
#include "vulkan_wrapper.h"
#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#include "vtk_mac.h"
#endif // __APPLE__
#endif // VTK_RUST_BINDGEN

#ifdef __cplusplus
#define _Bool bool
extern "C" {
#endif

struct VtkDeviceNative;
struct VtkWindowNative;

#ifdef __ANDROID__
// TODO
#elif defined __linux__
struct wl_display;
struct wl_registry;
struct xdg_wm_base;
struct wl_surface;
struct xdg_surface;
#endif

struct VtkContextNative {
#ifdef __APPLE__
    /** <div rustbindgen private> */
    NSApplication* ns_application;
#elif defined __ANDROID__
    // TODO:
#elif defined __linux__
    /** <div rustbindgen private> */
    struct wl_display* wayland_display;
    /** <div rustbindgen private> */
    struct wl_registry* wayland_registry;
    /** <div rustbindgen private> */
    struct wl_compositor* wayland_compositor;
    /** <div rustbindgen private> */
    struct xdg_wm_base* wayland_shell;
#endif
};

struct VtkDeviceNative {
    struct VtkContextNative* vtk_context;

    _Bool initialized;
    VkInstance vk_instance;
    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    uint32_t graphics_queue_family_idx;
    VkCommandPool vk_command_pool;
    VkQueue vk_queue;

    // One big vertex buffer, consisting of vertex_buffer_size bytes.
    VkBuffer vk_vertex_buffer;
    // The mapped host coherent pointer to the vertex buffer.
    void* vertex_buffer_ptr;
    // The number of bytes in vk_vertex_buffer.
    uint32_t vertex_buffer_size;
    // The device memory backing the vertex buffer.
    VkDeviceMemory vk_vertex_buffer_device_memory;
};

struct VtkWindowNative {
    struct VtkDeviceNative* vtk_device;

    VkSurfaceKHR vk_surface;
    enum VkFormat vk_surface_format;
    VkColorSpaceKHR vk_color_space;

    VkRenderPass vk_surface_render_pass;
    VkPipelineLayout vk_pipeline_layout;
    VkPipeline vk_pipeline;

    uint8_t num_swap_chain_images;
    VkSwapchainKHR vk_swapchain;
    VkImage* vk_swap_chain_images;
    VkImageView *vk_swap_chain_images_views;
    struct VkExtent2D vk_extent_2d;
    VkFramebuffer* vk_swap_chain_framebuffers;

    VkCommandBuffer *vk_command_buffers;
    uint32_t command_buffer_len;
    VkSemaphore vk_semaphore;
    VkFence vk_fence;

#ifdef __APPLE__
    /** Platform-specific data. <div rustbindgen private> */
    VtkViewController* vtk_view_controller;
    NSApplication* ns_application;
#elif defined __ANDROID__
    // TODO:
#elif defined __linux__
    struct wl_surface* wayland_surface;
    struct xdg_surface* wayland_shell_surface;
#endif
};


/** Null-terminated, static string. <div rustbindgen private> */
VkShaderModule vtk_compile_shader(VkDevice vk_device, uint8_t const* bytes, size_t size);

struct VtkContextNative* vtk_context_init();

// Run the event loop.
void vtk_context_run(struct VtkContextNative* context);

struct VtkDeviceNative* vtk_device_init(struct VtkContextNative* vtk_context);

struct VtkWindowNative* vtk_window_init(struct VtkDeviceNative* vtk_device);

void vtk_render_frame(struct VtkWindowNative* vtk_window);

#ifdef __cplusplus
}
#endif

#endif
