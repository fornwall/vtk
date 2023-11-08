#import "CustomViewController.h"
#import <QuartzCore/CAMetalLayer.h>
#import <CoreVideo/CVDisplayLink.h>

#include "rust-ffi.h"
#include "vulkan_main.h"

#include <MoltenVK/mvk_vulkan.h>

#pragma mark -
#pragma mark CustomViewController

@implementation CustomViewController {
    CVDisplayLinkRef _displayLink;
    // TODO: vulkan state struct Custom Custom;
    uint32_t _maxFrameCount;
    uint64_t _frameCount;
    BOOL _stop;
}

/** Since this is a single-view app, initialize Vulkan as view is appearing. */
-(void) viewWillAppear {
    [super viewWillAppear];

    self.view.wantsLayer = YES; // Back the view with a layer created by the makeBackingLayer method.
}

- (void)viewDidAppear {
    // If this value is set to zero, the Custom will render frames until the window is closed.
    // If this value is not zero, it establishes a maximum number of frames that will be
    // rendered, and once this count has been reached, the Custom will stop rendering.
    // Once rendering is finished, the Custom will delay cleaning up Vulkan objects until
    // the window is closed.
    _maxFrameCount = 0;
    _stop = NO;
    _frameCount = 0;

    init_window((const CAMetalLayer*) self.view.layer);

    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, self);
    CVDisplayLinkStart(_displayLink);
}

-(void)loadView
{
    self.view = [[CustomView alloc] init];
}

-(void) viewDidDisappear {
    _stop = YES;

    CVDisplayLinkRelease(_displayLink);
    // TODO: Custom_cleanup(&Custom);

    [super viewDidDisappear];
}

#pragma mark Display loop callback function

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
        void* display_link_context __attribute__((unused))) {
    CustomViewController* view_controller = (CustomViewController*) display_link_context;
    if (view_controller->_stop == NO) {
        // TODO: Draw Custom_draw(&CustomVC->Custom);
        //printf("Is vulkan ready? %s\n", is_vulkan_ready() ? "yes" : "no");
        if (is_vulkan_ready()) draw_frame();
        //view_controller->_stop = (view_controller->_maxFrameCount && ++view_controller->_frameCount >= view_controller->_maxFrameCount);
    }
    return kCVReturnSuccess;
}

@end


#pragma mark -
#pragma mark CustomView

@implementation CustomView

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
 * causes this Custom to replace the swapchain, in order to optimize rendering for the new resolution.
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
            exit(0);
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

@end
