#include <Cocoa/Cocoa.h>

#include "vulkan_wrapper.h"

#include "vtk_cffi.h"
#include "vtk_log.h"
#include "vtk_internal.h"

#import "vtk_mac.h"

struct VtkApplicationNative* vtk_application_init() {
    id ns_application = [NSApplication sharedApplication];
    [ns_application setActivationPolicy:NSApplicationActivationPolicyRegular];

    struct VtkApplicationNative* result = malloc(sizeof(struct VtkApplicationNative));
    result->ns_application = ns_application;
    return result;
}

void vtk_application_run(struct VtkApplicationNative* vtk_application) {
    NSApplication* ns_application = vtk_application->ns_application;
    [ns_application activateIgnoringOtherApps:YES];
    [ns_application run];
}

_Bool vtk_window_init_platform(struct VtkWindowNative* vtk_window) {
    NSRect frame = NSMakeRect(0, 0, 600, 600);
    VtkViewController* vtk_view_controller = [[VtkViewController alloc] init];

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

    // Locked down full screen:
    //[app setPresentationOptions:NSFullScreenWindowMask];
    // Normal full screen:
    //[window toggleFullScreen: nil];
    return true;
}
