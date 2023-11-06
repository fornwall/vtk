#import "Cocoa/Cocoa.h"
#import "DemoViewController.h"

int main(int argc, const char * argv[])
{
    [NSAutoreleasePool new];

    id app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect frame = NSMakeRect(0, 0, 300, 300);

    NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    id window = [[[NSWindow alloc] initWithContentRect:frame
                                             styleMask:windowStyle
                                               backing:NSBackingStoreBuffered
                                                 defer:NO] autorelease];
    [window cascadeTopLeftFromPoint:NSMakePoint(10, 10)];
    [window setTitle:@"Hello"];
    [window makeKeyAndOrderFront:nil];
    [window autorelease];

    DemoView* view = [[DemoView alloc] initWithFrame: frame];
    [window setContentView:view];

    [app activateIgnoringOtherApps:YES];
    [app run];

    return 0;
}
