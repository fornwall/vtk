#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>
#include <xdg-shell.h>

#define CHECK_WL_RESULT(_expr) \
if (!(_expr)) \
{ \
    printf("Error executing %s.", #_expr); \
}

static struct SwapchainElement* elements = NULL;
static struct wl_display* display = NULL;
static struct wl_registry* registry = NULL;
static struct wl_compositor* compositor = NULL;
static struct wl_surface* surface = NULL;
static struct xdg_wm_base* shell = NULL;
static struct xdg_surface* shellSurface = NULL;
static struct xdg_toplevel* toplevel = NULL;
static int quit = 0;
static int readyToResize = 0;
static int resize = 0;
static int newWidth = 0;
static int newHeight = 0;
static uint32_t width = 1600;
static uint32_t height = 900;
static uint32_t currentFrame = 0;
static uint32_t imageIndex = 0;
static uint32_t imageCount = 0;

static void handleRegistry(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);

static const struct wl_registry_listener registryListener = {
    .global = handleRegistry
};

static void handleShellPing(void* data, struct xdg_wm_base* shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener shellListener = {
    .ping = handleShellPing
};

static void handleShellSurfaceConfigure(void* data, struct xdg_surface* shellSurface, uint32_t serial)
{
    xdg_surface_ack_configure(shellSurface, serial);
    if (resize) {
        readyToResize = 1;
    }
}

static const struct xdg_surface_listener shellSurfaceListener = {
    .configure = handleShellSurfaceConfigure
};

static void handleToplevelConfigure(
    void* data,
    struct xdg_toplevel* toplevel,
    int32_t width,
    int32_t height,
    struct wl_array* states
) {
    if (width != 0 && height != 0)
    {
        resize = 1;
        newWidth = width;
        newHeight = height;
    }
}

static void handleToplevelClose(void* data, struct xdg_toplevel* toplevel) {
    quit = 1;
}

static const struct xdg_toplevel_listener toplevelListener = {
    .configure = handleToplevelConfigure,
    .close = handleToplevelClose
};

static void handleRegistry( void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        CHECK_WL_RESULT(compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        CHECK_WL_RESULT(shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(shell, &shellListener, NULL);
    }
}

int main(int argc, char** argv)
{
    CHECK_WL_RESULT(display = wl_display_connect(NULL));
    CHECK_WL_RESULT(registry = wl_display_get_registry(display));

    wl_registry_add_listener(registry, &registryListener, NULL);
    wl_display_roundtrip(display);

    CHECK_WL_RESULT(surface = wl_compositor_create_surface(compositor));
    CHECK_WL_RESULT(shellSurface = xdg_wm_base_get_xdg_surface(shell, surface));

    xdg_surface_add_listener(shellSurface, &shellSurfaceListener, NULL);

    CHECK_WL_RESULT(toplevel = xdg_surface_get_toplevel(shellSurface));

    xdg_toplevel_add_listener(toplevel, &toplevelListener, NULL);
    xdg_toplevel_set_title(toplevel, appName);
    xdg_toplevel_set_app_id(toplevel, appName);

    wl_surface_commit(surface);
    wl_display_roundtrip(display);
    wl_surface_commit(surface);

    // TODO: Vulkan init

    while (!quit)
    {
        if (readyToResize && resize) {
            width = newWidth;
            height = newHeight;

            CHECK_VK_RESULT(vkDeviceWaitIdle(device));

            destroySwapchain();
            createSwapchain();

            currentFrame = 0;
            imageIndex = 0;

            readyToResize = 0;
            resize = 0;

            wl_surface_commit(surface);
        }

        // TODO: vulkan render

        wl_display_roundtrip(display);
    }

    CHECK_VK_RESULT(vkDeviceWaitIdle(device));

    destroySwapchain();

    vkDestroyCommandPool(device, commandPool, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, vulkanSurface, NULL);
    GET_EXTENSION_FUNCTION(vkDestroyDebugUtilsMessengerEXT)(instance, debugMessenger, NULL);
    vkDestroyInstance(instance, NULL);

    xdg_toplevel_destroy(toplevel);
    xdg_surface_destroy(shellSurface);
    wl_surface_destroy(surface);
    xdg_wm_base_destroy(shell);
    wl_compositor_destroy(compositor);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}

