#import "Cocoa/Cocoa.h"
#import "CustomViewController.h"

int main(int _argc __attribute__((unused)), const char * _argv[] __attribute__((unused)))
{
    [NSAutoreleasePool new];

    id app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect frame = NSMakeRect(0, 0, 600, 600);
    CustomViewController * viewController = [[CustomViewController alloc] init];
    //[self.window setContentView:viewController.view];
    //CustomView* view = [[CustomView alloc] initWithFrame: frame];
    NSView* view = viewController.view;

    NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    id window = [[[NSWindow alloc] initWithContentRect:frame
                                             styleMask:windowStyle
                                               backing:NSBackingStoreBuffered
                                                 defer:NO] autorelease];
    [window autorelease];
    [window cascadeTopLeftFromPoint:NSMakePoint(10, 10)];
    [window makeKeyAndOrderFront:nil];
    [window setContentView:view];
    [window setTitle:@"Vulkan"];


    [app activateIgnoringOtherApps:YES];
    [window makeFirstResponder: view];
    //[window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [((NSWindow*)window) center];
    //[app setPresentationOptions:NSFullScreenWindowMask];
    [app run];

    return 0;
}
