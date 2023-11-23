#include <Cocoa/Cocoa.h>

#include "vtk_cffi.h"
#include "vtk_internal.h"
#include "vtk_log.h"
#include "vtk_mac.h"
#include "vulkan_wrapper.h"
#include "rustffi.h"

struct VtkContextNative* vtk_context_init(void* vtk_application) {
#ifdef VTK_VULKAN_VALIDATION
    setenv("VK_LOADER_DEBUG", "all", 1);
#endif
    id ns_application = [NSApplication sharedApplication];
    [ns_application setActivationPolicy:NSApplicationActivationPolicyRegular];

    vtk_load_vulkan_symbols();

    struct VtkContextNative* result = malloc(sizeof(struct VtkContextNative));
    result->ns_application = ns_application;
    result->vtk_application = vtk_application;
    return result;
}

void vtk_context_run(struct VtkContextNative* vtk_context) {
    NSApplication* ns_application = vtk_context->ns_application;
    [ns_application activateIgnoringOtherApps:YES];
    [ns_application run];
}

// Rendering loop callback function for use with a CVDisplayLink.
// https://developer.apple.com/documentation/corevideo/cvdisplaylinkoutputcallback
static CVReturn DisplayLinkCallback(
        // A display link that requests a frame.
        CVDisplayLinkRef display_link __attribute__((unused)),
        // A pointer to the current time.
        const CVTimeStamp* _in_now __attribute__((unused)),
        // A pointer to the display time for a frame.
        const CVTimeStamp* _in_output_time __attribute__((unused)),
        // Currently unused. Pass 0.
        CVOptionFlags _flags_in __attribute__((unused)),
        // Currently unused. Pass 0 __attribute__((unused)).
        CVOptionFlags* _flags_out __attribute__((unused)),
        // A pointer to app-defined data.
        void* display_link_context) {
    struct VtkWindowNative* vtk_window = (struct VtkWindowNative*) display_link_context;
    struct VtkDeviceNative* vtk_device = vtk_window->vtk_device;
    // TODO: Check if aborting..
        vtk_application_render_frame(vtk_device->vtk_context->vtk_application, vtk_device, vtk_window);
        vtk_render_frame(vtk_window);
       // Aborting check done
    return kCVReturnSuccess;
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

    vtk_application_setup_window(vtk_device->vtk_context->vtk_application, vtk_device, vtk_window);

    CVDisplayLinkRef _displayLink;
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, vtk_window);
    CVDisplayLinkStart(_displayLink);
    return true;
}
