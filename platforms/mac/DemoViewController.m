#import "DemoViewController.h"
#import <QuartzCore/CAMetalLayer.h>
#import <CoreVideo/CVDisplayLink.h>

#include <rust-ffi.h>

#include <MoltenVK/mvk_vulkan.h>
// #include "../../Vulkan-Tools/cube/cube.c"


#pragma mark -
#pragma mark DemoViewController

@implementation DemoViewController {
    CVDisplayLinkRef _displayLink;
    // TODO: vulkan state struct demo demo;
    uint32_t _maxFrameCount;
    uint64_t _frameCount;
    BOOL _stop;
}

/** Since this is a single-view app, initialize Vulkan as view is appearing. */
-(void) viewWillAppear {
    [super viewWillAppear];

    self.view.wantsLayer = YES;		// Back the view with a layer created by the makeBackingLayer method.

    // If this value is set to zero, the demo will render frames until the window is closed.
    // If this value is not zero, it establishes a maximum number of frames that will be
    // rendered, and once this count has been reached, the demo will stop rendering.
    // Once rendering is finished, the demo will delay cleaning up Vulkan objects until
    // the window is closed.
    _maxFrameCount = 0;

    // TODO: Setup vulkan
    // demo_main(&demo, self.view.layer, argc, argv);

    _stop = NO;
    _frameCount = 0;
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, self);
    CVDisplayLinkStart(_displayLink);
}

-(void) viewDidDisappear {
    _stop = YES;

    CVDisplayLinkRelease(_displayLink);
    // TODO: demo_cleanup(&demo);

    [super viewDidDisappear];
}



#pragma mark Display loop callback function

/** Rendering loop callback function for use with a CVDisplayLink. */
static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
        const CVTimeStamp* now,
        const CVTimeStamp* outputTime,
        CVOptionFlags flagsIn,
        CVOptionFlags* flagsOut,
        void* target) {
    DemoViewController* demoVC =(DemoViewController*)target;
    if ( !demoVC->_stop ) {
        // TODO: Draw demo_draw(&demoVC->demo);
        demoVC->_stop = (demoVC->_maxFrameCount && ++demoVC->_frameCount >= demoVC->_maxFrameCount);
    }
    return kCVReturnSuccess;
}

@end


#pragma mark -
#pragma mark DemoView

@implementation DemoView

/** Indicates that the view wants to draw using the backing layer instead of using drawRect:.  */
-(BOOL) wantsUpdateLayer { return YES; }

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

/** If the wantsLayer property is set to YES, this method will be invoked to return a layer instance. */
-(CALayer*) makeBackingLayer {
    CALayer* layer = [self.class.layerClass layer];
    CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    layer.contentsScale = MIN(viewScale.width, viewScale.height);
    return layer;
}

/**
 * If this view moves to a screen that has a different resolution scale (eg. Standard <=> Retina),
 * update the contentsScale of the layer, which will trigger a Vulkan VK_SUBOPTIMAL_KHR result, which
 * causes this demo to replace the swapchain, in order to optimize rendering for the new resolution.
 */
-(BOOL) layer: (CALayer *)layer shouldInheritContentsScale: (CGFloat)newScale fromWindow: (NSWindow *)window {
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

-(void) flagsChanged: (NSEvent*) theEvent
{
  if( theEvent.modifierFlags & NSEventModifierFlagShift ) {
      printf("SHIFT HELD\n");
  }
}

-(void) keyDown: (NSEvent*) theEvent
{
  NSString* pressedKeyString = theEvent.charactersIgnoringModifiers;
  if (pressedKeyString.length == 0) {
      // Reject dead keys.
  }
  unichar pressedKey = [pressedKeyString characterAtIndex: 0];
  switch (pressedKey) {
    case NSUpArrowFunctionKey:
        printf("UP ARROW\n");
        add_held_keys(Key_ArrowUp);
        break;
    case NSDownArrowFunctionKey:
        printf("Down ARROW\n");
        break;
    case 'a':
        printf("A HELD\n");
        break;
    //pressedKeys.insert( pressedKey );
  }
}

@end
