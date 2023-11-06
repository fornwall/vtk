#ifndef __VULKAN_MAIN_H__
#define __VULKAN_MAIN_H__

#include <game-activity/native_app_glue/android_native_app_glue.h>

// Initialize vulkan device context. After returning, vulkan is ready to draw.
bool init_window(struct android_app* app);

// Delete vulkan device context when the application goes away.
void terminate_window();

// Check if vulkan is ready to draw.
bool is_vulkan_ready();

// Ask Vulkan to Render a frame.
bool draw_frame();

#endif

