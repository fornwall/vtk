#include <Cocoa/Cocoa.h>

#include "vtk_cffi.h"
#include "vtk_internal.h"
#include "vtk_log.h"
#include "vtk_mac.h"
#include "vulkan_wrapper.h"
#include "rustffi.h"

struct VtkContextNative* vtk_context_init() {
#ifdef VTK_VULKAN_VALIDATION
    setenv("VK_LOADER_DEBUG", "all", 1);
#endif
    id ns_application = [NSApplication sharedApplication];
    [ns_application setActivationPolicy:NSApplicationActivationPolicyRegular];

    vtk_load_vulkan_symbols();

    struct VtkContextNative* result = malloc(sizeof(struct VtkContextNative));
    result->ns_application = ns_application;
    return result;
}

void vtk_context_run(struct VtkContextNative* vtk_context) {
    NSApplication* ns_application = vtk_context->ns_application;
    [ns_application activateIgnoringOtherApps:YES];
    [ns_application run];
}

_Bool vtk_window_init_platform(struct VtkWindowNative* vtk_window) {
    NSRect frame = NSMakeRect(0, 0, 600, 600);
    VtkViewController* vtk_view_controller = [[VtkViewController alloc] init];
    vtk_view_controller.view.wantsLayer = true;

    vtk_window->vtk_view_controller = vtk_view_controller;
    vtk_view_controller->vtk_window = vtk_window;

    VtkView* vtk_view = (VtkView*) vtk_view_controller.view;

    NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    id window = [[[NSWindow alloc] initWithContentRect:frame
                                             styleMask:windowStyle
                                               backing:NSBackingStoreBuffered
                                                 defer:NO] autorelease];
    [window autorelease];
    [window cascadeTopLeftFromPoint:NSMakePoint(10, 10)];
    [window makeKeyAndOrderFront:nil];
    [window setContentView:vtk_view];
    [window setTitle:@"Vulkan"];

    [window makeFirstResponder: vtk_view];

    [((NSWindow*)window) center];

    const CAMetalLayer* metal_layer = (const CAMetalLayer*) vtk_view.layer;

    VkMetalSurfaceCreateInfoEXT surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .pLayer = metal_layer,
    };

    struct VtkDeviceNative* vtk_device = vtk_window->vtk_device;
    CALL_VK(vkCreateMetalSurfaceEXT(vtk_device->vk_instance, &surface_create_info, NULL, &vtk_window->vk_surface));
    vtk_setup_window_rendering(vtk_window);

    return true;
}
