#ifndef VTK_PLATFORM_H_INCLUDED

#ifdef __APPLE__
#define VTK_PLATFORM_APPLE
#elif defined __ANDROID__
#define VTK_PLATFORM_ANDROID
#elif defined __linux__
#define VTK_PLATFORM_WAYLAND
#elif defined WIN32
#define VTK_PLATFORM_WINDOWS
#else
#error Unsupported platform
#endif

#endif
