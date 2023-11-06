/*
 * File: OSXWindow.m
 *
 * Brief:
 *  Creates a OSX/Cocoa application and window without Interface Builder or XCode.
 *
 * Compile with:
 *  cc OSXWindow.m -o OSXWindow -framework Cocoa
 */

#import "Cocoa/Cocoa.h"

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
    /*
    NSWindow * window = [[NSWindow alloc] initWithContentRect:windowRect
                                          styleMask:windowStyle
                                          backing:NSBackingStoreBuffered
                                          defer:NO];
                                          */
    [window cascadeTopLeftFromPoint:NSMakePoint(10, 10)];
    [window setTitle:@"Hello"];
    [window makeKeyAndOrderFront:nil];
    [window autorelease];

    // Window controller:
    //NSWindowController * windowController = [[NSWindowController alloc] initWithWindow:window];
    //[windowController autorelease];

    // This will add a simple text view to the window,
    // so we can write a test string on it.
    //NSTextView * textView = [[NSTextView alloc] initWithFrame:windowRect];
    //[textView autorelease];

    //[window setContentView:textView];
    //[textView insertText:@"Hello OSX/Cocoa world!"];

    // TODO: Create app delegate to handle system events.
    // TODO: Create menus (especially Quit!)

    // Show window and run event loop.
    //[window orderFrontRegardless];

    [app activateIgnoringOtherApps:YES];
    [app run];

    //[NSApp run];

    //[pool drain];

    return 0;
}
