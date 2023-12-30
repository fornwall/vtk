#ifndef VTK_MAC_H_INCLUDED
#define VTK_MAC_H_INCLUDED

#import <AppKit/AppKit.h>

#pragma mark -
#pragma mark VtkViewController

/** The main view controller. */
@interface VtkViewController : NSViewController {
@public
  struct VtkWindowNative *vtk_window;
}
@end

#pragma mark -
#pragma mark VtkView

/** Metal-compatible view. */
@interface VtkView : NSView
@end

#endif
