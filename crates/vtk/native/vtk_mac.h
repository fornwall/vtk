#ifndef VTK_MAC_H_INCLUDED
#define VTK_MAC_H_INCLUDED

#import <AppKit/AppKit.h>
//#include "vtk_cffi.h"

struct VtkViewController;

#pragma mark -
#pragma mark VtkViewController

/** The main view controller. */
@interface VtkViewController : NSViewController {
    @public
    struct VtkWindowNative* vtk_window;
    CVDisplayLinkRef _displayLink;
    // TODO: vulkan state struct Vtk Vtk;
    uint32_t _maxFrameCount;
    uint64_t _frameCount;
    BOOL _stop;
}
@end

#pragma mark -
#pragma mark VtkView

/** Metal-compatibile view. */
@interface VtkView : NSView
@end

#endif