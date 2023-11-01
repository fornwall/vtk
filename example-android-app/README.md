# Vulkan Example

Create a triangle and draw it to the screen

## Android vulkan implementation

The `vkCreateAndroidSurfaceKHRfunction is implemented as

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
