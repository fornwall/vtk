#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include "vtk_cffi.h"
#include "vtk_internal.h"
#include "vtk_log.h"
#include "vulkan_wrapper.h"

#define VTK_CHECK_WL_RESULT(_expr) \
if (!(_expr)) { fprintf(stderr, "[vtk] Error executing %s.", #_expr); exit(1); }

static void handleRegistry(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);

static const struct wl_registry_listener vtk_wl_registry_listener = {
    .global = handleRegistry
};

static void handleShellPing(void* data, struct xdg_wm_base* shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener shellListener = {
    .ping = handleShellPing
};

static void handleShellSurfaceConfigure(void* data, struct xdg_surface* shell_surface, uint32_t serial)
{
    xdg_surface_ack_configure(shell_surface, serial);
    //if (resize) {
        //readyToResize = 1;
    //}
}

static const struct xdg_surface_listener shellSurfaceListener = {
    .configure = handleShellSurfaceConfigure
};

static void handleToplevelConfigure(void* data, struct xdg_toplevel* toplevel, int32_t width, int32_t height, struct wl_array* states) {
    if (width != 0 && height != 0) {
        LOGE("handleToplevelClose called: %d, %d", width, height);
        //resize = 1;
        //newWidth = width;
        //newHeight = height;
    }
}

static void handleToplevelClose(void* data, struct xdg_toplevel* toplevel) {
    // quit = 1;
    LOGE("handleToplevelClose called");
}

static const struct xdg_toplevel_listener toplevelListener = {
    .configure = handleToplevelConfigure,
    .close = handleToplevelClose
};

static void handleRegistry(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    struct VtkContextNative* vtk_context_native = (struct VtkContextNative*) data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        VTK_CHECK_WL_RESULT(vtk_context_native->wayland_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        VTK_CHECK_WL_RESULT(vtk_context_native->wayland_shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(vtk_context_native->wayland_shell, &shellListener, NULL);
    }
}

struct VtkContextNative* vtk_context_init() {
  vtk_load_vulkan_symbols();

  struct VtkContextNative* result = malloc(sizeof(struct VtkContextNative));
  VTK_CHECK_WL_RESULT(result->wayland_display = wl_display_connect(NULL));
  VTK_CHECK_WL_RESULT(result->wayland_registry = wl_display_get_registry(result->wayland_display));
  result->wayland_compositor = NULL;
  result->wayland_shell = NULL;

  // TODO: Avoid static listener. use inline or save in result struct.
  wl_registry_add_listener(result->wayland_registry, &vtk_wl_registry_listener, result);
  wl_display_roundtrip(result->wayland_display);

  // These fields should be set from the listener:
  assert(result->wayland_compositor != NULL);
  assert(result->wayland_shell != NULL);

  return result;
}

void vtk_context_run(struct VtkContextNative* vtk_context) {
    // TODO
}


_Bool vtk_window_init_platform(struct VtkWindowNative* vtk_window) {
  struct VtkDeviceNative* vtk_device = vtk_window->vtk_device;
  struct VtkContextNative* vtk_context = vtk_device->vtk_context;

  VTK_CHECK_WL_RESULT(vtk_window->wayland_surface = wl_compositor_create_surface(vtk_context->wayland_compositor));
  VTK_CHECK_WL_RESULT(vtk_window->wayland_shell_surface = xdg_wm_base_get_xdg_surface(vtk_context->wayland_shell, vtk_window->wayland_surface));

  xdg_surface_add_listener(vtk_window->wayland_shell_surface, &shellSurfaceListener, NULL);

  static struct xdg_toplevel* toplevel;
  VTK_CHECK_WL_RESULT(toplevel = xdg_surface_get_toplevel(vtk_window->wayland_shell_surface));

  const char* appName = "vtk TODO";
  xdg_toplevel_add_listener(toplevel, &toplevelListener, NULL);
  xdg_toplevel_set_title(toplevel, appName);
  xdg_toplevel_set_app_id(toplevel, appName);

  wl_surface_commit(vtk_window->wayland_surface);
  wl_display_roundtrip(vtk_context->wayland_display);
  wl_surface_commit(vtk_window->wayland_surface);

  VkWaylandSurfaceCreateInfoKHR surface_create_info = {
    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
    .display = vtk_context->wayland_display,
    .surface = vtk_window->wayland_surface,
  };
  CALL_VK(vkCreateWaylandSurfaceKHR(vtk_device->vk_instance, &surface_create_info, NULL, &vtk_window->vk_surface));

  vtk_setup_window_rendering(vtk_window);

  return 1;
}
