#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <android/rect.h>
#include <android/surface_control.h>
#include <android/window.h>

void android_main(ANativeActivity *activity, void *savedState, size_t savedStateSize) {}

struct VtkContextNative *vtk_context_init() {
#ifdef VTK_VULKAN_VALIDATION
  setenv("VK_LOADER_DEBUG", "all", 1);
#endif
  id ns_application = [NSApplication sharedApplication];
  [ns_application setActivationPolicy:NSApplicationActivationPolicyRegular];

  vtk_load_vulkan_symbols();

  struct VtkContextNative *result = malloc(sizeof(struct VtkContextNative));
  result->ns_application = ns_application;
  return result;
}
