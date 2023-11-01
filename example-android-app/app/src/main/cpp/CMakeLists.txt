cmake_minimum_required(VERSION 3.27.7)

project(vktuts)

find_package(game-activity REQUIRED CONFIG)

# build vulkan app
get_filename_component(REPO_ROOT_DIR ${CMAKE_SOURCE_DIR}/../../../../..  ABSOLUTE)
set(COMMON_DIR ${REPO_ROOT_DIR}/common)

add_library(${CMAKE_PROJECT_NAME} SHARED
        vulkan_main.cpp
    vulkan_wrapper.cpp
    AndroidMain.cpp
    GameActivitySources.cpp)

include_directories(${COMMON_DIR}/vulkan_wrapper)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall \
                     -DVK_USE_PLATFORM_ANDROID_KHR")
target_link_libraries(${CMAKE_PROJECT_NAME}
    game-activity::game-activity_static
    log
    android)
