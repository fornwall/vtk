#ifndef __VULKANMAIN_HPP__
#define __VULKANMAIN_HPP__

#include <game-activity/native_app_glue/android_native_app_glue.h>

// Initialize vulkan device context. After return, vulkan is ready to draw
bool init_window(android_app* app);

// delete vulkan device context when application goes away
void terminate_window(void);

// Check if vulkan is ready to draw
bool IsVulkanReady(void);

// Ask Vulkan to Render a frame
bool draw_frame(void);

#endif

