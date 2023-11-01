# Vulkan Example

Create a triangle and draw it to the screen

## Android vulkan implementation

The `vkCreateAndroidSurfaceKHRfunction` is implemented as

```c++
VkResult CreateAndroidSurfaceKHR(VkInstance instance, VkAndroidSurfaceCreateInfoKHR* pCreateInfo, VkAllocationCallbacks* allocator, VkSurfaceKHR* out_surface) {
    if (!allocator) allocator = &GetData(instance).allocator;
    void* mem = allocator->pfnAllocation(allocator->pUserData, sizeof(Surface), alignof(Surface), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    Surface* surface = new (mem) Surface;

    surface->window = pCreateInfo->window;
    surface->swapchain_handle = VK_NULL_HANDLE;
    surface->used_by_swapchain = false;
    native_window_get_consumer_usage(surface->window.get(), &surface->consumer_usage);
    native_window_api_connect(surface->window.get(), NATIVE_WINDOW_API_EGL);
    *out_surface = HandleFromSurface(surface);
    return VK_SUCCESS;
}
```

where

```c++
struct Surface {
    android::sp<ANativeWindow> window;
    VkSwapchainKHR swapchain_handle;
    uint64_t consumer_usage;

    // Indicate whether this surface has been used by a swapchain, no matter the
    // swapchain is still current or has been destroyed.
    bool used_by_swapchain;
};
```

About surfaces - [https://source.android.com/docs/core/graphics/arch-sh](https://source.android.com/docs/core/graphics/arch-sh):

> A surface is an interface for a producer to exchange buffers with a consumer.
> 
> The BufferQueue for a display surface is typically configured for triple-buffering. Buffers are allocated on demand, so if the producer generates buffers slowly enough, such as at > 30 fps on a 60 fps display, there might only be two allocated buffers in the queue. Allocating buffers on demand helps minimize memory consumption. You can see a summary of the buffers associated with every layer in the dumpsys SurfaceFlinger output.
> 
> Most clients render onto surfaces using OpenGL ES or Vulkan. However, some clients render onto surfaces using a canvas.

Now, going towards the swap chain, we query [vkGetPhysicalDeviceSurfaceCapabilitiesKHR](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceCapabilitiesKHR.html) to get a [VkSurfaceCapabilitiesKHR](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html):

```c
typedef struct VkSurfaceCapabilitiesKHR {
    uint32_t                         minImageCount;
    uint32_t                         maxImageCount;
    VkExtent2D                       currentExtent;
    VkExtent2D                       minImageExtent;
    VkExtent2D                       maxImageExtent;
    uint32_t                         maxImageArrayLayers;
    VkSurfaceTransformFlagsKHR       supportedTransforms;
    VkSurfaceTransformFlagBitsKHR    currentTransform;
    VkCompositeAlphaFlagsKHR         supportedCompositeAlpha;
    VkImageUsageFlags                supportedUsageFlags;
} VkSurfaceCapabilitiesKHR;
```

This `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` function is implemented here: https://cs.android.com/android/platform/superproject/+/master:frameworks/native/vulkan/libvulkan/swapchain.cpp;l=860

```c++
        if (pPresentMode && IsSharedPresentMode(pPresentMode->presentMode)) {
            capabilities->minImageCount = 1;
            capabilities->maxImageCount = 1;
        } else if (pPresentMode && pPresentMode->presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            capabilities->minImageCount =
                std::min(max_buffer_count, min_undequeued_buffers + 2);
            capabilities->maxImageCount = static_cast<uint32_t>(max_buffer_count);
        } else {
            capabilities->minImageCount =
                std::min(max_buffer_count, min_undequeued_buffers + 1);
            capabilities->maxImageCount = static_cast<uint32_t>(max_buffer_count);
        }
```
