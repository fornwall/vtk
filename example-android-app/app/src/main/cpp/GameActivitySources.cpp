#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

// Glue from GameActivity to android_main()
// Passing GameActivity event from main thread to app native thread.
extern  "C" {
    #include <game-activity/native_app_glue/android_native_app_glue.c>
}
