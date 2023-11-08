#ifndef __VULKAN_MAIN_H__
#define __VULKAN_MAIN_H__

#include <stdbool.h>

#ifdef __ANDROID__
# include <game-activity/native_app_glue/android_native_app_glue.h>
#elif defined __APPLE__
# import <QuartzCore/CAMetalLayer.h>
#endif

// Initialize vulkan device context. After returning, vulkan is ready to draw.
void init_window(
#ifdef __ANDROID__
        struct android_app *app
#elif defined __APPLE__
        const CAMetalLayer* metal_layer
#else
        struct wl_surface* wayland_surface,
        struct wl_surface* wayland_display
#endif
        );

// Delete vulkan device context when the application goes away.
void terminate_window();

// Check if vulkan is ready to draw.
bool is_vulkan_ready();

// Ask Vulkan to Render a frame.
void draw_frame();

#endif

