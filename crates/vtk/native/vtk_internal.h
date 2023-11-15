#ifndef VTK_INTERNAL_H_INCLUDED
#define VTK_INTERNAL_H_INCLUDED

_Bool vtk_window_init_platform(struct VtkWindowNative* vtk_window);

void vtk_create_swap_chain(struct VtkWindowNative* vtk_window);

void vtk_delete_swap_chain(struct VtkWindowNative* vtk_window);

void vtk_draw_frame(struct VtkWindowNative* vtk_window);

#endif