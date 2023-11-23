#import <QuartzCore/CAMetalLayer.h>
#import <CoreVideo/CVDisplayLink.h>

// Gamepad - but extract to own files
// #import <IOKit/hid/IOHIDLib.h>

#include "vtk_cffi.h"
#include "vtk_mac.h"
#include "vtk_log.h"
#include "vtk_internal.h"
#include "rustffi.h"

// #include <MoltenVK/mvk_vulkan.h>

#pragma mark -
#pragma mark VtkViewController

@implementation VtkViewController

-(void)loadView
{
    self.view = [[VtkView alloc] init];
}

-(void) viewDidDisappear {
    //CVDisplayLinkRelease(_displayLink);
    // TODO: Vtk_cleanup(&Vtk);
    [super viewDidDisappear];
}

#pragma mark Display loop callback function


@end

#pragma mark -
#pragma mark VtkView

@implementation VtkView

/** Indicates that the view wants to draw using the backing layer instead of using drawRect:.  */
-(BOOL) wantsUpdateLayer { return YES; }

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

/** If the wantsLayer property is set to YES, this method will be invoked to return a layer instance. */
-(CALayer*) makeBackingLayer {
    CAMetalLayer* layer = [self.class.layerClass layer];
    //CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    //layer.contentsScale = MIN(viewScale.width, viewScale.height);
    return layer;
}

/**
 * If this view moves to a screen that has a different resolution scale (eg. Standard <=> Retina),
 * update the contentsScale of the layer, which will trigger a Vulkan VK_SUBOPTIMAL_KHR result, which
 * causes this Vtk to replace the swapchain, in order to optimize rendering for the new resolution.
 */
- (BOOL)layer: (CALayer*)layer shouldInheritContentsScale:(CGFloat)newScale fromWindow:(NSWindow*)window {
    if (newScale == layer.contentsScale) { return NO; }

    layer.contentsScale = newScale;
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

void update_pressed_keys(Key* pressed_keys, uint32_t mac_identifier, bool pressed) {
    Key key;
    switch (mac_identifier) {
        case NSUpArrowFunctionKey:
            key = Key_ArrowUp;
            break;
        case NSRightArrowFunctionKey:
            key = Key_ArrowRight;
            break;
        case NSDownArrowFunctionKey:
            key = Key_ArrowDown;
            break;
        case NSLeftArrowFunctionKey:
            key = Key_ArrowLeft;
            break;
        case 'a':
            key = Key_A;
            break;
        case 's':
            key = Key_A;
            break;
        default:
            return;
    }

    if (pressed) {
      pressed_keys->bits |= key.bits;
    } else {
      pressed_keys->bits &= ~key.bits;
    }
}

- (void)keyDown: (NSEvent*)theEvent
{
  NSString* pressedKeyString = theEvent.charactersIgnoringModifiers;
  if (pressedKeyString.length == 0) {
      // Reject dead keys.
  }
  unichar pressedKey = [pressedKeyString characterAtIndex: 0];
  Key pressed_keys = {
      .bits = 0
  };
  switch (pressedKey) {
    case NSUpArrowFunctionKey:
        pressed_keys.bits |= Key_ArrowUp.bits;
        add_held_keys(Key_ArrowUp);
        break;
    case NSRightArrowFunctionKey:
        add_held_keys(Key_ArrowRight);
        break;
    case NSDownArrowFunctionKey:
        add_held_keys(Key_ArrowDown);
        break;
    case NSLeftArrowFunctionKey:
        add_held_keys(Key_ArrowLeft);
        break;
    case 'a':
        add_held_keys(Key_A);
        break;
    case 'd':
        add_held_keys(Key_D);
        break;
    case 's':
        add_held_keys(Key_S);
        break;
    case 'w':
        add_held_keys(Key_W);
        break;
    case 'f':
        if ((theEvent.modifierFlags & (NSEventModifierFlagCommand | NSEventModifierFlagControl)) == (NSEventModifierFlagCommand | NSEventModifierFlagControl)) {
          [[self window] toggleFullScreen: nil];
        } else {
          printf("f HELD\n");
        }
        break;
    case 'q':
        if ((theEvent.modifierFlags & NSEventModifierFlagCommand) == NSEventModifierFlagCommand) {
            [[self window] close];
        }
        break;
    //pressedKeys.insert( pressedKey );
  }
}

-(void) flagsChanged: (NSEvent*) theEvent
{
  if (theEvent.modifierFlags & NSEventModifierFlagShift) {
        add_held_keys(Key_Shift);
  }
  if (theEvent.modifierFlags & NSEventModifierFlagCommand) {
        add_held_keys(Key_Command);
  }
  if (theEvent.modifierFlags & NSEventModifierFlagCapsLock) {
        add_held_keys(Key_CapsLock);
  }
  if (theEvent.modifierFlags & NSEventModifierFlagControl) {
        add_held_keys(Key_Control);
  }
}

- (void)mouseDown:(NSEvent *)theEvent {
    NSPoint clicked_position = [theEvent locationInWindow]; // [self convertPoint:[theEvent locationInWindow] fromView:nil];
    printf("Clicked: %f, %f\n", clicked_position.x, clicked_position.y);
}

- (void)mouseUp:(NSEvent *)theEvent {
    // TODO
}

@end
