#include <Cocoa/Cocoa.h>

#include "vulkan_wrapper.h"

#include "vtk_cffi.h"
#include "vtk_log.h"
#include "vtk_internal.h"

#import "VtkViewController.h"

struct VtkMacApplication {
    NSApplication* ns_application;
};

struct VtkMacWindow {
    VtkViewController* vtk_view_controller;
    VtkView* vtk_view;
    NSApplication* ns_application;
};

struct VtkApplicationNative* vtk_application_init() {
    id ns_application = [NSApplication sharedApplication];
    [ns_application setActivationPolicy:NSApplicationActivationPolicyRegular];

    struct VtkMacApplication* vtk_mac_application = malloc(sizeof(struct VtkMacApplication));
    vtk_mac_application->ns_application = ns_application;

    struct VtkApplicationNative* result = malloc(sizeof(struct VtkApplicationNative));
    result->platform_specific = vtk_mac_application;
    return result;
}

void vtk_application_run(struct VtkApplicationNative* application) {
    struct VtkMacApplication* vtk_mac_application = (struct VtkMacApplication*) application->platform_specific;
    NSApplication* ns_application = vtk_mac_application->ns_application;
    [ns_application activateIgnoringOtherApps:YES];
    [ns_application run];
}

_Bool vtk_window_init_platform(struct VtkWindowNative* vtk_window) {
    NSRect frame = NSMakeRect(0, 0, 600, 600);
    VtkViewController* vtk_view_controller = [[VtkViewController alloc] init];

    struct VtkMacWindow* vtk_mac_window = malloc(sizeof(struct VtkMacWindow));
    vtk_mac_window->vtk_view_controller = vtk_view_controller;
    vtk_window->platform_specific = vtk_mac_window;
    vtk_view_controller->vtk_window = vtk_window;
    printf("device in init_platform: %p\n", vtk_window->vtk_device);
    printf("device in init_platform: %p\n", vtk_view_controller->vtk_window->vtk_device);

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

    // Locked down full screen:
    //[app setPresentationOptions:NSFullScreenWindowMask];
    // Normal full screen:
    //[window toggleFullScreen: nil];
    return true;
}
