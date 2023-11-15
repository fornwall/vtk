#include <assert.h>
#include <stdbool.h>

#ifdef __ANDROID__

#include <android/log.h>

static const char *kTAG = "vtk";
#define VA_ARGS(...) , ##__VA_ARGS__
# define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG VA_ARGS(__VA_ARGS__)))
# define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG VA_ARGS(__VA_ARGS__)))
# define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG VA_ARGS(__VA_ARGS__)))
# define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                          \
    __android_log_print(ANDROID_LOG_ERROR, "Tutorial ",                \
                        "Vulkan error. File[%s], line[%d]", __FILE__,  \
                        __LINE__);                                     \
    assert(false);                                                     \
  }

#else /* ifdef __ANDROID__ */

#include <stdio.h>

#define VA_ARGS(...) , ##__VA_ARGS__
# define LOGI(x, ...) \
  ((void)fprintf(stderr, "[info] " x "\n" VA_ARGS(__VA_ARGS__)))
# define LOGW(x, ...) \
  ((void)fprintf(stderr, "[warn] " x "\n" VA_ARGS(__VA_ARGS__)))
# define LOGE(x, ...) \
  ((void)fprintf(stderr, "[error] " x "\n" VA_ARGS(__VA_ARGS__)))
# define CALL_VK(func)                                                    \
  if (VK_SUCCESS != (func)) {                                             \
    fprintf(stderr, "[error] Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                        \
    assert(false);                                                        \
  }

#endif
