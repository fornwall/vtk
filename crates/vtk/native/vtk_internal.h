#ifndef VTK_INTERNAL_H_INCLUDED
#define VTK_INTERNAL_H_INCLUDED

_Bool vtk_window_init_platform(struct VtkWindowNative* vtk_window);

void vtk_setup_window_rendering(struct VtkWindowNative* vtk_window);

void vtk_tear_down_window_rendering(struct VtkWindowNative* vtk_window);

#endif