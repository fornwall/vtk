#import "VtkViewController.h"
#import <QuartzCore/CAMetalLayer.h>
#import <CoreVideo/CVDisplayLink.h>

// Gamepad - but extract to own files
// #import <IOKit/hid/IOHIDLib.h>

#include "vtk_log.h"
#include "rustffi.h"
#include "vulkan_main.h"

#include <MoltenVK/mvk_vulkan.h>

#pragma mark -
#pragma mark VtkViewController

@implementation VtkViewController

/** Since this is a single-view app, initialize Vulkan as view is appearing. */
-(void)viewWillAppear {
    [super viewWillAppear];

    self.view.wantsLayer = YES; // Back the view with a layer created by the makeBackingLayer method.

    struct VtkDeviceNative* vtk_device = vtk_window->vtk_device;
    printf("vtk_window in viewWill: %p\n", vtk_window);
    printf("vtk_device in viewWill: %p\n", vtk_device);
}

- (void)viewDidAppear {
    printf("vtk_window->width in view: %p\n", vtk_window->width);
    printf("vtk_window->height in view: %p\n", vtk_window->height);
    struct VtkDeviceNative* vtk_device = vtk_window->vtk_device;
    printf("vtk_window in view: %p\n", vtk_window);
    printf("vtk_device in view: %p\n", vtk_device);

    // If this value is set to zero, the Vtk will render frames until the window is closed.
    // If this value is not zero, it establishes a maximum number of frames that will be
    // rendered, and once this count has been reached, the Vtk will stop rendering.
    // Once rendering is finished, the Vtk will delay cleaning up Vulkan objects until
    // the window is closed.
    _maxFrameCount = 0;
    _stop = NO;
    _frameCount = 0;

    //printf("vk_instance in view: %p\n", vtk_device->vk_instance);
    const CAMetalLayer* metal_layer = (const CAMetalLayer*) self.view.layer;
    printf("Metal layer: %p\n", metal_layer);

    VkMetalSurfaceCreateInfoEXT surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .pLayer = metal_layer,
    };

    CALL_VK(vkCreateMetalSurfaceEXT(vtk_device->vk_instance, &surface_create_info, NULL, &vtk_window->vk_surface));
    //init_window((const CAMetalLayer*) self.view.layer);

    printf("viewDidAppear 4\n");
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, self);
    CVDisplayLinkStart(_displayLink);
    printf("viewDidAppear 5\n");
}

-(void)loadView
{
    self.view = [[VtkView alloc] init];
}

-(void) viewDidDisappear {
    _stop = YES;

    CVDisplayLinkRelease(_displayLink);
    // TODO: Vtk_cleanup(&Vtk);

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
    VtkViewController* view_controller = (VtkViewController*) display_link_context;
    if (view_controller->_stop == NO) {
        // TODO: Draw Vtk_draw(&VtkVC->Vtk);
        //printf("Is vulkan ready? %s\n", is_vulkan_ready() ? "yes" : "no");
        //if (is_vulkan_ready()) draw_frame();
        //view_controller->_stop = (view_controller->_maxFrameCount && ++view_controller->_frameCount >= view_controller->_maxFrameCount);
    }
    return kCVReturnSuccess;
}

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
