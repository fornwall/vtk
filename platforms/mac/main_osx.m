#import "Cocoa/Cocoa.h"
#import "CustomViewController.h"

int main(int _argc __attribute__((unused)), const char * _argv[] __attribute__((unused)))
{
    [NSAutoreleasePool new];

    id app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect frame = NSMakeRect(0, 0, 300, 300);
    CustomView* view = [[CustomView alloc] initWithFrame: frame];

    NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    id window = [[[NSWindow alloc] initWithContentRect:frame
                                             styleMask:windowStyle
                                               backing:NSBackingStoreBuffered
                                                 defer:NO] autorelease];
    [window autorelease];
    [window cascadeTopLeftFromPoint:NSMakePoint(10, 10)];
    [window makeKeyAndOrderFront:nil];
    [window setContentView:view];
    [window setTitle:@"Hello"];

    [app activateIgnoringOtherApps:YES];
    [window makeFirstResponder: view];
    [app run];

    return 0;
}
