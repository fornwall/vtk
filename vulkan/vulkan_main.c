#include "vulkan_main.h"
#include "vulkan_wrapper.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef __ANDROID__
#include <android/log.h>
static const char *kTAG = "vulkan-example";
# define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
# define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
# define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))
# define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, "Tutorial ",               \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }
#else
#include <stdio.h>
# define LOGI(x, ...) \
  ((void)fprintf(stderr, "[info] " x "\n", __VA_ARGS__))
# define LOGW(x, ...) \
  ((void)fprintf(stderr, "[warn] " x "\n", __VA_ARGS__))
# define LOGE(x, ...) \
  ((void)fprintf(stderr, "[error] " x "\n", __VA_ARGS__))
# define CALL_VK(func)                                                   \
  if (VK_SUCCESS != (func)) {                                            \
    fprintf(stderr, "[error] Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                       \
    assert(false);                                                       \
  }
#endif

#define VK_WRAP_ARRAY_SIZE(x) ((int)(sizeof(x) / sizeof((x)[0])))
#define VK_WRAP_ALLOC_ARRAY(element_type, array_size) ((element_type*) malloc(sizeof(element_type) * array_size))

struct VulkanDeviceInfo {
    bool initialized_;

    VkInstance vk_instance;
    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    uint32_t queueFamilyIndex_;

    VkSurfaceKHR vk_surface;
    VkQueue vk_queue;
};

struct VulkanDeviceInfo device;

struct VulkanSwapchainInfo {
    VkSwapchainKHR vk_swapchain;
    uint32_t num_images;

    VkExtent2D vk_extend_2d;
    VkFormat vk_format;

    // array of frame buffers and views
    VkImage *vk_images;
    VkImageView *vk_image_views;
    VkFramebuffer *vk_framebuffers;
};

struct VulkanSwapchainInfo swapchain;

struct VulkanBufferInfo {
    VkBuffer vk_buffer;
};

struct VulkanBufferInfo buffers;

struct VulkanGfxPipelineInfo {
    VkPipelineLayout vk_pipeline_layout;
    VkPipelineCache vk_pipeline_cache;
    VkPipeline vk_pipeline;
};

struct VulkanGfxPipelineInfo gfxPipeline;

struct VulkanRenderInfo {
    VkRenderPass vk_render_pass;
    VkCommandPool vk_command_pool;
    VkCommandBuffer *vk_command_buffer;
    uint32_t command_buffer_len;
    VkSemaphore vk_semaphore;
    VkFence vk_fence;
};

struct VulkanRenderInfo render;

/*
 * set_image_layout():
 *    Helper function to transition color buffer layout
 */
void set_image_layout(VkCommandBuffer cmdBuffer, VkImage image,
                      VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags destStages);

void create_vulkan_device(
#ifdef __ANDROID__
        ANativeWindow *native_window,
#elif defined __APPLE__
        CAMetalLayer const* metal_layer,
#else
        struct wl_display* wayland_display,
        struct wl_surface* wayland_surface,
#endif
        VkApplicationInfo *appInfo) {
    const char *instance_extensions[] = {
            "VK_KHR_surface",
#ifdef __ANDROID__
            "VK_KHR_android_surface",
#elif defined __APPLE__
            "VK_MVK_macos_surface",
#else
            "VK_KHR_wayland_surface",
#endif
    };

    char const *enabledLayerNames[] = {
#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
            "VK_LAYER_KHRONOS_validation"
#endif
    };

    VkInstanceCreateInfo instance_create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = NULL,
            .pApplicationInfo = appInfo,
            .enabledLayerCount = VK_WRAP_ARRAY_SIZE(enabledLayerNames),
            .ppEnabledLayerNames = enabledLayerNames,
            .enabledExtensionCount = VK_WRAP_ARRAY_SIZE(instance_extensions),
            .ppEnabledExtensionNames = instance_extensions,
    };

    CALL_VK(vkCreateInstance(&instance_create_info, NULL, &device.vk_instance));

#ifdef __ANDROID__
    VkAndroidSurfaceCreateInfoKHR surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
            .window = native_window
    };
    CALL_VK(vkCreateAndroidSurfaceKHR(device.vk_instance, &surface_create_info, NULL, &device.vk_surface));
#elif defined __APPLE__
    VkMetalSurfaceCreateInfoEXT surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
            .pNext = NULL,
            .flags = 0,
            .pLayer = metal_layer
    };
    CALL_VK(vkCreateMetalSurfaceEXT(device.vk_instance, &surface_create_info, NULL, &device.vk_surface));
#else
    VkWaylandSurfaceCreateInfoKHR surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = wayland_display,
        .surface = wayland_surface
    };
    CALL_VK(vkCreateWaylandSurfaceKHR(instance, &surface_create_info, NULL, &device.vk_surface));
#endif

    uint32_t gpuCount = 0;
    CALL_VK(vkEnumeratePhysicalDevices(device.vk_instance, &gpuCount, NULL));

    VkPhysicalDevice tmpGpus[gpuCount];
    CALL_VK(vkEnumeratePhysicalDevices(device.vk_instance, &gpuCount, tmpGpus));
    // Select the first device:
    device.vk_physical_device = tmpGpus[0];

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device.vk_physical_device, &queueFamilyCount, NULL);
    assert(queueFamilyCount);
    VkQueueFamilyProperties *queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device.vk_physical_device, &queueFamilyCount, queueFamilyProperties);

    uint32_t queue_family_idx;
    for (queue_family_idx = 0; queue_family_idx < queueFamilyCount; queue_family_idx++) {
        if (queueFamilyProperties[queue_family_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    free(queueFamilyProperties);
    assert(queue_family_idx < queueFamilyCount);
    device.queueFamilyIndex_ = queue_family_idx;

    float priorities[] = {1.0f};

    VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = queue_family_idx,
            .queueCount = 1,
            .pQueuePriorities = priorities,
    };

    char const *device_extensions[] = {
            "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo deviceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = NULL,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = NULL,
            .enabledExtensionCount = VK_WRAP_ARRAY_SIZE(device_extensions),
            .ppEnabledExtensionNames = device_extensions,
            .pEnabledFeatures = NULL,
    };

    CALL_VK(vkCreateDevice(device.vk_physical_device, &deviceCreateInfo, NULL, &device.vk_device));
    vkGetDeviceQueue(device.vk_device, device.queueFamilyIndex_, 0, &device.vk_queue);
}

void create_swap_chain() {
    memset(&swapchain, 0, sizeof(swapchain));

    VkSurfaceCapabilitiesKHR vk_surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.vk_physical_device, device.vk_surface, &vk_surface_capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.vk_physical_device, device.vk_surface, &format_count, NULL);
    VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *) malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.vk_physical_device, device.vk_surface, &format_count, formats);
    LOGI("Got %d formats", format_count);

    uint32_t chosenFormat;
    for (chosenFormat = 0; chosenFormat < format_count; chosenFormat++) {
        if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM) break;
    }
    assert(chosenFormat < format_count);

    swapchain.vk_extend_2d = vk_surface_capabilities.currentExtent;
    swapchain.vk_format = formats[chosenFormat].format;

    VkSurfaceCapabilitiesKHR surfaceCap;
    CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.vk_physical_device, device.vk_surface, &surfaceCap))
    assert(surfaceCap.supportedCompositeAlpha | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);

    LOGE("minImageCount = %d", vk_surface_capabilities.minImageCount);
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = NULL,
            .surface = device.vk_surface,
            .minImageCount = vk_surface_capabilities.minImageCount,
            .imageFormat = formats[chosenFormat].format,
            .imageColorSpace = formats[chosenFormat].colorSpace,
            .imageExtent = vk_surface_capabilities.currentExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &device.queueFamilyIndex_,
            // Handle rotation ourselves for best performance, see:
            // https://developer.android.com/games/optimize/vulkan-prerotation
            .preTransform = vk_surface_capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
    };

    CALL_VK(vkCreateSwapchainKHR(device.vk_device, &swapchainCreateInfo, NULL, &swapchain.vk_swapchain))

    CALL_VK(vkGetSwapchainImagesKHR(device.vk_device, swapchain.vk_swapchain, &swapchain.num_images, NULL))

    swapchain.vk_images = (VkImage *) malloc(sizeof(VkImage) * swapchain.num_images);
    CALL_VK(vkGetSwapchainImagesKHR(device.vk_device, swapchain.vk_swapchain, &swapchain.num_images, swapchain.vk_images))

    swapchain.vk_image_views = (VkImageView *) malloc(sizeof(VkImageView) * swapchain.num_images);
    for (uint32_t i = 0; i < swapchain.num_images; i++) {
        VkImageViewCreateInfo vk_image_view_create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .image = swapchain.vk_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = swapchain.vk_format,
                .components = {
                        .r = VK_COMPONENT_SWIZZLE_R,
                        .g = VK_COMPONENT_SWIZZLE_G,
                        .b = VK_COMPONENT_SWIZZLE_B,
                        .a = VK_COMPONENT_SWIZZLE_A,
                },
                .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                },
        };
        CALL_VK(vkCreateImageView(device.vk_device, &vk_image_view_create_info, NULL, &swapchain.vk_image_views[i]))
    }

    free(formats);
}

void delete_swap_chain() {
    for (uint32_t i = 0; i < swapchain.num_images; i++) {
        vkDestroyFramebuffer(device.vk_device, swapchain.vk_framebuffers[i], NULL);
        vkDestroyImageView(device.vk_device, swapchain.vk_image_views[i], NULL);
    }
    vkDestroySwapchainKHR(device.vk_device, swapchain.vk_swapchain, NULL);

    free(swapchain.vk_images);
    free(swapchain.vk_image_views);
    free(swapchain.vk_framebuffers);
}

void create_frame_buffers(VkRenderPass vk_render_pass) {
    VkImageView depth_view = VK_NULL_HANDLE;
    // create a framebuffer from each swapchain image
    swapchain.vk_framebuffers = (VkFramebuffer *) malloc(sizeof(VkFramebuffer) * swapchain.num_images);
    for (uint32_t i = 0; i < swapchain.num_images; i++) {
        VkImageView attachments[2] = {
                swapchain.vk_image_views[i],
                depth_view,
        };
        VkFramebufferCreateInfo vk_frame_buffer_create_info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = NULL,
                .renderPass = vk_render_pass,
                .attachmentCount = 1,  // 2 if using depth
                .pAttachments = attachments,
                .width = (uint32_t) swapchain.vk_extend_2d.width,
                .height = (uint32_t) swapchain.vk_extend_2d.height,
                .layers = 1,
        };
        vk_frame_buffer_create_info.attachmentCount = (depth_view == VK_NULL_HANDLE ? 1 : 2);

        CALL_VK(vkCreateFramebuffer(device.vk_device, &vk_frame_buffer_create_info, NULL,
                                    &swapchain.vk_framebuffers[i]));
    }
}

// A helper function
bool MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask,
                          uint32_t *typeIndex) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device.vk_physical_device, &memoryProperties);
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags & requirements_mask) ==
                requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

void create_vertex_buffer() {
    // -----------------------------------------------
    // Create the triangle vertex buffer

    // Vertex positions
    const float vertexData[] = {
            -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    };

    // Create a vertex buffer
    VkBufferCreateInfo createBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .size = sizeof(vertexData),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &device.queueFamilyIndex_,
    };

    CALL_VK(vkCreateBuffer(device.vk_device, &createBufferInfo, NULL,
                           &buffers.vk_buffer))

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device.vk_device, buffers.vk_buffer, &memReq);

    VkMemoryAllocateInfo vk_memory_allocation_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = memReq.size,
            .memoryTypeIndex = 0,  // Memory type assigned in the next step
    };

    // Assign the proper memory type for that buffer
    MapMemoryTypeToIndex(memReq.memoryTypeBits,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &vk_memory_allocation_info.memoryTypeIndex);

    // Allocate memory for the buffer
    VkDeviceMemory deviceMemory;
    CALL_VK(vkAllocateMemory(device.vk_device, &vk_memory_allocation_info, NULL, &deviceMemory))

    void *data;
    CALL_VK(vkMapMemory(device.vk_device, deviceMemory, 0, vk_memory_allocation_info.allocationSize,
                        0, &data))
    memcpy(data, vertexData, sizeof(vertexData));
    vkUnmapMemory(device.vk_device, deviceMemory);

    CALL_VK(vkBindBufferMemory(device.vk_device, buffers.vk_buffer, deviceMemory, 0))
}

void delete_buffers(void) {
    vkDestroyBuffer(device.vk_device, buffers.vk_buffer, NULL);
}

void load_shader_from_file(const char *filePath, VkShaderModule *shaderOut) {
#ifdef __ANDROID__
    // struct android_app *androidAppCtx = NULL;
    assert(androidAppCtx);
    AAsset *file = AAssetManager_open(androidAppCtx->activity->assetManager, filePath, AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);

    char *fileContent = malloc(fileLength);

    AAsset_read(file, fileContent, fileLength);
    AAsset_close(file);

    VkShaderModuleCreateInfo vk_shader_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .codeSize = fileLength,
            .pCode = (const uint32_t *) fileContent,
    };
    CALL_VK(vkCreateShaderModule(device.vk_device, &vk_shader_module_create_info, NULL, shaderOut));

    free(fileContent);
#endif
}

void create_graphics_pipeline() {
    memset(&gfxPipeline, 0, sizeof(gfxPipeline));
    // Create pipeline layout (empty)
    VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .setLayoutCount = 0,
            .pSetLayouts = NULL,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = NULL,
    };
    CALL_VK(vkCreatePipelineLayout(device.vk_device, &vk_pipeline_layout_create_info, NULL, &gfxPipeline.vk_pipeline_layout));

    VkShaderModule vertexShader, fragmentShader;
    load_shader_from_file("shaders/tri.vert.spv", &vertexShader);
    load_shader_from_file("shaders/tri.frag.spv", &fragmentShader);

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shader_stages[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = NULL,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertexShader,
                    .pName = "main",
                    .pSpecializationInfo = NULL,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = NULL,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragmentShader,
                    .pName = "main",
                    .pSpecializationInfo = NULL,
            }};

    VkViewport viewports = {
            .x = 0,
            .y = 0,
            .width = (float) swapchain.vk_extend_2d.width,
            .height = (float) swapchain.vk_extend_2d.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
            .offset = {.x = 0, .y = 0},
            .extent = swapchain.vk_extend_2d,
    };

    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = NULL,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = NULL,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state
    VkPipelineColorBlendAttachmentState attachmentStates = {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = NULL,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1,
    };

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = NULL,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings[] = {
            {
                    .binding = 0,
                    .stride = 3 * sizeof(float),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            }
    };

    VkVertexInputAttributeDescription vertex_input_attributes[] = {
            {
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = 0,
            }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = NULL,
            .vertexBindingDescriptionCount = VK_WRAP_ARRAY_SIZE(vertex_input_bindings),
            .pVertexBindingDescriptions = vertex_input_bindings,
            .vertexAttributeDescriptionCount = VK_WRAP_ARRAY_SIZE(vertex_input_attributes),
            .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .stageCount = 2,
            .pStages = shader_stages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = NULL,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = NULL,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = NULL,
            .layout = gfxPipeline.vk_pipeline_layout,
            .renderPass = render.vk_render_pass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };

    VkPipelineCacheCreateInfo pipelineCacheInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,  // reserved, must be 0
            .initialDataSize = 0,
            .pInitialData = NULL,
    };

    CALL_VK(vkCreatePipelineCache(device.vk_device, &pipelineCacheInfo, NULL, &gfxPipeline.vk_pipeline_cache));

    CALL_VK(vkCreateGraphicsPipelines(
            device.vk_device, gfxPipeline.vk_pipeline_cache, 1, &pipelineCreateInfo, NULL,
            &gfxPipeline.vk_pipeline))

    // We don't need the shaders anymore, we can release their memory
    vkDestroyShaderModule(device.vk_device, vertexShader, NULL);
    vkDestroyShaderModule(device.vk_device, fragmentShader, NULL);
}

void delete_graphics_pipeline(void) {
    if (gfxPipeline.vk_pipeline == VK_NULL_HANDLE) return;
    vkDestroyPipeline(device.vk_device, gfxPipeline.vk_pipeline, NULL);
    vkDestroyPipelineCache(device.vk_device, gfxPipeline.vk_pipeline_cache, NULL);
    vkDestroyPipelineLayout(device.vk_device, gfxPipeline.vk_pipeline_layout, NULL);
}

// init_window:
//   Initialize Vulkan Context when android application window is created
//   upon return, vulkan is ready to draw frames
void init_window(
#ifdef __ANDROID__
        struct android_app *app
#elif defined __APPLE__
        const CAMetalLayer* metal_layer
#else
        struct wl_surface* wayland_surface,
        struct wl_surface* wayland_display
#endif
) {
#ifdef __ANDROID__
    if (!load_vulkan_symbols()) {
        LOGW("Vulkan is unavailable, install vulkan and re-start");
        return;
    }
#endif

    VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = NULL,
            .pApplicationName = "vulkan-example",
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
            .pEngineName = "vulkan-example",
            .engineVersion = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };

    create_vulkan_device(
#ifdef __ANDROID__
            app->window,
#elif defined __APPLE__
            metal_layer,
#else
            wayland_display,
            wayland_surface,
#endif
            &app_info);

    create_swap_chain();

    VkAttachmentDescription vk_attachment_description = {
            .format = swapchain.vk_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference vk_attachment_reference = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription vk_subpass_description = {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = NULL,
            .colorAttachmentCount = 1,
            .pColorAttachments = &vk_attachment_reference,
            .pResolveAttachments = NULL,
            .pDepthStencilAttachment = NULL,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = NULL,
    };

    VkRenderPassCreateInfo vk_render_pass_create_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = NULL,
            .attachmentCount = 1,
            .pAttachments = &vk_attachment_description,
            .subpassCount = 1,
            .pSubpasses = &vk_subpass_description,
            .dependencyCount = 0,
            .pDependencies = NULL,
    };
    CALL_VK(vkCreateRenderPass(device.vk_device, &vk_render_pass_create_info, NULL, &render.vk_render_pass))

    create_frame_buffers(render.vk_render_pass);

    create_vertex_buffer();

    create_graphics_pipeline();

    VkCommandPoolCreateInfo vk_command_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            // There are two possible flags for command pools:
            // - VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands
            //    very often (may change memory allocation behavior).
            // - VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually,
            //   without this flag they all have to be reset together.
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = device.queueFamilyIndex_,
    };
    CALL_VK(vkCreateCommandPool(device.vk_device, &vk_command_pool_create_info, NULL, &render.vk_command_pool));

    // Record a command buffer that just clear the screen
    // 1 command buffer draw in 1 framebuffer
    // In our case we create one command buffer per swap chain image.
    render.command_buffer_len = swapchain.num_images;
    render.vk_command_buffer = VK_WRAP_ALLOC_ARRAY(VkCommandBuffer, swapchain.num_images);
    VkCommandBufferAllocateInfo vk_command_buffer_allocate_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = render.vk_command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = render.command_buffer_len,
    };
    CALL_VK(vkAllocateCommandBuffers(device.vk_device, &vk_command_buffer_allocate_info, render.vk_command_buffer));

    for (uint32_t bufferIndex = 0; bufferIndex < swapchain.num_images; bufferIndex++) {
        // We start by creating and declare the "beginning" our command buffer
        VkCommandBufferBeginInfo vk_command_buffer_begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = NULL,
                .flags = 0,
                .pInheritanceInfo = NULL,
        };
        CALL_VK(vkBeginCommandBuffer(render.vk_command_buffer[bufferIndex], &vk_command_buffer_begin_info));
        // transition the display image to color attachment layout
        set_image_layout(render.vk_command_buffer[bufferIndex],
                         swapchain.vk_images[bufferIndex],
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        // Now we start a renderpass. Any draw command has to be recorded in a renderpass.
        VkClearValue vk_clear_value = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}};
        VkRenderPassBeginInfo vk_render_pass_begin_info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = NULL,
                .renderPass = render.vk_render_pass,
                .framebuffer = swapchain.vk_framebuffers[bufferIndex],
                .renderArea = {.offset = {.x = 0, .y = 0,}, .extent = swapchain.vk_extend_2d},
                .clearValueCount = 1,
                .pClearValues = &vk_clear_value
        };

        vkCmdBeginRenderPass(render.vk_command_buffer[bufferIndex], &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(render.vk_command_buffer[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, gfxPipeline.vk_pipeline);

            uint32_t first_binding = 0;
            uint32_t binding_count = 1;
            VkDeviceSize buffer_offset = 0;
            vkCmdBindVertexBuffers(render.vk_command_buffer[bufferIndex], first_binding, binding_count, &buffers.vk_buffer, &buffer_offset);

            uint32_t vertex_count = 3;
            uint32_t instance_count = 1;
            uint32_t first_vertex = 0;
            uint32_t first_instance = 0;
            vkCmdDraw(render.vk_command_buffer[bufferIndex], vertex_count, instance_count, first_vertex, first_instance);
        }
        vkCmdEndRenderPass(render.vk_command_buffer[bufferIndex]);
        CALL_VK(vkEndCommandBuffer(render.vk_command_buffer[bufferIndex]));
    }

    // We need to create a fence to be able, in the main loop, to wait for our
    // draw command(s) to finish before swapping the framebuffers
    VkFenceCreateInfo vk_fence_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    CALL_VK(vkCreateFence(device.vk_device, &vk_fence_create_info, NULL, &render.vk_fence));

    // We need to create a semaphore to be able to wait, in the main loop, for our
    // framebuffer to be available for us before drawing.
    VkSemaphoreCreateInfo vk_semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
    };
    CALL_VK(vkCreateSemaphore(device.vk_device, &vk_semaphore_create_info, NULL, &render.vk_semaphore));

    device.initialized_ = true;
}

bool is_vulkan_ready() {
    return device.initialized_;
}

void terminate_window() {
    vkFreeCommandBuffers(device.vk_device, render.vk_command_pool, render.command_buffer_len, render.vk_command_buffer);
    free(render.vk_command_buffer);

    vkDestroyCommandPool(device.vk_device, render.vk_command_pool, NULL);
    vkDestroyRenderPass(device.vk_device, render.vk_render_pass, NULL);
    delete_swap_chain();
    delete_graphics_pipeline();
    delete_buffers();

    vkDestroyDevice(device.vk_device, NULL);
    vkDestroyInstance(device.vk_instance, NULL);

    device.initialized_ = false;
}

void draw_frame() {
    CALL_VK(vkWaitForFences(device.vk_device, 1, &render.vk_fence, VK_TRUE, UINT64_MAX))
    CALL_VK(vkResetFences(device.vk_device, 1, &render.vk_fence))

    uint32_t acquired_image_idx;
    CALL_VK(vkAcquireNextImageKHR(device.vk_device, swapchain.vk_swapchain, UINT64_MAX, render.vk_semaphore, VK_NULL_HANDLE, &acquired_image_idx))

    VkPipelineStageFlags vk_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render.vk_semaphore,
            .pWaitDstStageMask = &vk_pipeline_stage_flags,
            .commandBufferCount = 1,
            .pCommandBuffers = &render.vk_command_buffer[acquired_image_idx],
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = NULL
    };
    CALL_VK(vkQueueSubmit(device.vk_queue, 1, &submit_info, render.vk_fence))

    VkResult result;
    VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = NULL,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = NULL,
            .swapchainCount = 1,
            .pSwapchains = &swapchain.vk_swapchain,
            .pImageIndices = &acquired_image_idx,
            .pResults = &result,
    };
    CALL_VK(vkQueuePresentKHR(device.vk_queue, &presentInfo))
}

/*
 * set_image_layout():
 *    Helper function to transition color buffer layout
 */
void set_image_layout(VkCommandBuffer cmdBuffer,
                      VkImage image,
                      VkImageLayout oldImageLayout,
                      VkImageLayout newImageLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags destStages) {
    VkImageMemoryBarrier vk_image_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = oldImageLayout,
            .newLayout = newImageLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };

    switch (oldImageLayout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;
        default:
            break;
    }

    switch (newImageLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;
        default:
            break;
    }

    vkCmdPipelineBarrier(cmdBuffer, srcStages, destStages, 0, 0, NULL, 0, NULL, 1, &vk_image_memory_barrier);
}
