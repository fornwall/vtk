#include "vulkan_main.h"
#include "vulkan_wrapper.h"

#include <android/log.h>

#include <cassert>
#include <cstring>
#include <vector>

// Android log function wrappers
static const char *kTAG = "Vulkan-Tutorial05";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, "Tutorial ",               \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

struct VulkanDeviceInfo {
    bool initialized_;

    VkInstance vk_instance;
    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    uint32_t queueFamilyIndex_;

    VkSurfaceKHR vk_surface;
    VkQueue vk_queue;
};

VulkanDeviceInfo device;

struct VulkanSwapchainInfo {
    VkSwapchainKHR vk_swapchain;
    uint32_t swap_chain_length;

    VkExtent2D vk_extend_2d;
    VkFormat vk_format;

    // array of frame buffers and views
    VkImage *vk_images;
    VkImageView *vk_image_views;
    VkFramebuffer *vk_framebuffers;
};

VulkanSwapchainInfo swapchain;

struct VulkanBufferInfo {
    VkBuffer vk_buffer;
};

VulkanBufferInfo buffers;

struct VulkanGfxPipelineInfo {
    VkPipelineLayout vk_pipeline_layout;
    VkPipelineCache vk_pipeline_cache;
    VkPipeline vk_pipeline;
};

VulkanGfxPipelineInfo gfxPipeline;

struct VulkanRenderInfo {
    VkRenderPass vk_render_pass;
    VkCommandPool vk_command_pool;
    VkCommandBuffer* vk_command_buffer;
    uint32_t command_buffer_len;
    VkSemaphore vk_semaphore;
    VkFence vk_fence;
};

VulkanRenderInfo render;

android_app *androidAppCtx = nullptr;

/*
 * set_image_layout():
 *    Helper function to transition color buffer layout
 */
void set_image_layout(VkCommandBuffer cmdBuffer, VkImage image,
                      VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags destStages);

void create_vulkan_device(ANativeWindow *native_window, VkApplicationInfo *appInfo) {
    const char *instance_extensions[] = {
            "VK_KHR_surface",
#ifdef __ANDROID__
            "VK_KHR_android_surface",
#endif
    };

    VkInstanceCreateInfo instance_create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = sizeof(instance_extensions) / sizeof(instance_extensions[0]),
            .ppEnabledExtensionNames = instance_extensions,
    };

    CALL_VK(vkCreateInstance(&instance_create_info, nullptr, &device.vk_instance));

#ifdef __ANDROID__
    VkAndroidSurfaceCreateInfoKHR surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = native_window
    };

    CALL_VK(vkCreateAndroidSurfaceKHR(device.vk_instance, &surface_create_info, nullptr, &device.vk_surface));
#else
# error Unsupported platform
#endif

    uint32_t gpuCount = 0;
    CALL_VK(vkEnumeratePhysicalDevices(device.vk_instance, &gpuCount, nullptr));

    VkPhysicalDevice tmpGpus[gpuCount];
    CALL_VK(vkEnumeratePhysicalDevices(device.vk_instance, &gpuCount, tmpGpus));
    // Select the first device:
    device.vk_physical_device = tmpGpus[0];

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device.vk_physical_device, &queueFamilyCount, nullptr);
    assert(queueFamilyCount);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device.vk_physical_device, &queueFamilyCount, queueFamilyProperties.data());

    uint32_t queue_family_idx;
    for (queue_family_idx = 0; queue_family_idx < queueFamilyCount; queue_family_idx++) {
        if (queueFamilyProperties[queue_family_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    assert(queue_family_idx < queueFamilyCount);
    device.queueFamilyIndex_ = queue_family_idx;

    float priorities[] = {1.0f};

    VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queue_family_idx,
            .queueCount = 1,
            .pQueuePriorities = priorities,
    };

    char const* device_extensions[] = {
            "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo deviceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]),
            .ppEnabledExtensionNames = device_extensions,
            .pEnabledFeatures = nullptr,
    };

    CALL_VK(vkCreateDevice(device.vk_physical_device, &deviceCreateInfo, nullptr, &device.vk_device));
    vkGetDeviceQueue(device.vk_device, device.queueFamilyIndex_, 0, &device.vk_queue);
}

void create_swap_chain() {
    memset(&swapchain, 0, sizeof(swapchain));

    VkSurfaceCapabilitiesKHR vk_surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.vk_physical_device, device.vk_surface, &vk_surface_capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.vk_physical_device, device.vk_surface, &format_count, nullptr);
    VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[format_count];
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
    CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.vk_physical_device, device.vk_surface, &surfaceCap));
    assert(surfaceCap.supportedCompositeAlpha | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);

    LOGE("minImageCount = %d", vk_surface_capabilities.minImageCount);
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
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

    CALL_VK(vkCreateSwapchainKHR(device.vk_device, &swapchainCreateInfo, nullptr, &swapchain.vk_swapchain));

    CALL_VK(vkGetSwapchainImagesKHR(device.vk_device, swapchain.vk_swapchain, &swapchain.swap_chain_length, nullptr));

    swapchain.vk_images = (VkImage *) malloc(sizeof(VkImage) * swapchain.swap_chain_length);
    CALL_VK(vkGetSwapchainImagesKHR(device.vk_device, swapchain.vk_swapchain, &swapchain.swap_chain_length, swapchain.vk_images));

    swapchain.vk_image_views = (VkImageView *) malloc(sizeof(VkImageView) * swapchain.swap_chain_length);
    for (uint32_t i = 0; i < swapchain.swap_chain_length; i++) {
        VkImageViewCreateInfo viewCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
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
        CALL_VK(vkCreateImageView(device.vk_device, &viewCreateInfo, nullptr, &swapchain.vk_image_views[i]));
    }

    delete[] formats;
}

void delete_swap_chain() {
    for (int i = 0; i < swapchain.swap_chain_length; i++) {
        vkDestroyFramebuffer(device.vk_device, swapchain.vk_framebuffers[i], nullptr);
        vkDestroyImageView(device.vk_device, swapchain.vk_image_views[i], nullptr);
    }
    vkDestroySwapchainKHR(device.vk_device, swapchain.vk_swapchain, nullptr);

    free(swapchain.vk_images);
    free(swapchain.vk_image_views);
    free(swapchain.vk_framebuffers);
}

void create_frame_buffers(VkRenderPass &renderPass, VkImageView depthView = VK_NULL_HANDLE) {
    // create a framebuffer from each swapchain image
    swapchain.vk_framebuffers = (VkFramebuffer *) malloc(sizeof(VkFramebuffer) * swapchain.swap_chain_length);
    for (uint32_t i = 0; i < swapchain.swap_chain_length; i++) {
        VkImageView attachments[2] = {
                swapchain.vk_image_views[i],
                depthView,
        };
        VkFramebufferCreateInfo fbCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .renderPass = renderPass,
                .attachmentCount = 1,  // 2 if using depth
                .pAttachments = attachments,
                .width = static_cast<uint32_t>(swapchain.vk_extend_2d.width),
                .height = static_cast<uint32_t>(swapchain.vk_extend_2d.height),
                .layers = 1,
        };
        fbCreateInfo.attachmentCount = (depthView == VK_NULL_HANDLE ? 1 : 2);

        CALL_VK(vkCreateFramebuffer(device.vk_device, &fbCreateInfo, nullptr,
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

// Create our vertex buffer
bool CreateBuffers() {
    // -----------------------------------------------
    // Create the triangle vertex buffer

    // Vertex positions
    const float vertexData[] = {
            -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    };

    // Create a vertex buffer
    VkBufferCreateInfo createBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = sizeof(vertexData),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &device.queueFamilyIndex_,
    };

    CALL_VK(vkCreateBuffer(device.vk_device, &createBufferInfo, nullptr,
                           &buffers.vk_buffer));

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device.vk_device, buffers.vk_buffer, &memReq);

    VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memReq.size,
            .memoryTypeIndex = 0,  // Memory type assigned in the next step
    };

    // Assign the proper memory type for that buffer
    MapMemoryTypeToIndex(memReq.memoryTypeBits,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &allocInfo.memoryTypeIndex);

    // Allocate memory for the buffer
    VkDeviceMemory deviceMemory;
    CALL_VK(vkAllocateMemory(device.vk_device, &allocInfo, nullptr, &deviceMemory));

    void *data;
    CALL_VK(vkMapMemory(device.vk_device, deviceMemory, 0, allocInfo.allocationSize,
                        0, &data));
    memcpy(data, vertexData, sizeof(vertexData));
    vkUnmapMemory(device.vk_device, deviceMemory);

    CALL_VK(
            vkBindBufferMemory(device.vk_device, buffers.vk_buffer, deviceMemory, 0));
    return true;
}

void delete_buffers(void) {
    vkDestroyBuffer(device.vk_device, buffers.vk_buffer, nullptr);
}

enum ShaderType {
    VERTEX_SHADER, FRAGMENT_SHADER
};

VkResult loadShaderFromFile(const char *filePath, VkShaderModule *shaderOut, ShaderType type) {
    // Read the file
    assert(androidAppCtx);
    AAsset *file = AAssetManager_open(androidAppCtx->activity->assetManager, filePath, AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);

    char *fileContent = new char[fileLength];

    AAsset_read(file, fileContent, fileLength);
    AAsset_close(file);

    VkShaderModuleCreateInfo shaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = fileLength,
            .pCode = (const uint32_t *) fileContent,
    };
    VkResult result = vkCreateShaderModule(device.vk_device, &shaderModuleCreateInfo, nullptr, shaderOut);
    assert(result == VK_SUCCESS);

    delete[] fileContent;

    return result;
}

VkResult CreateGraphicsPipeline() {
    memset(&gfxPipeline, 0, sizeof(gfxPipeline));
    // Create pipeline layout (empty)
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };
    CALL_VK(vkCreatePipelineLayout(device.vk_device, &pipelineLayoutCreateInfo, nullptr, &gfxPipeline.vk_pipeline_layout));

    VkShaderModule vertexShader, fragmentShader;
    loadShaderFromFile("shaders/tri.vert.spv", &vertexShader, VERTEX_SHADER);
    loadShaderFromFile("shaders/tri.frag.spv", &fragmentShader, FRAGMENT_SHADER);

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shaderStages[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertexShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragmentShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            }};

    VkViewport viewports{
            .x = 0,
            .y = 0,
            .width = (float) swapchain.vk_extend_2d.width,
            .height = (float) swapchain.vk_extend_2d.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
            .offset {.x = 0, .y = 0,},
            .extent = swapchain.vk_extend_2d,
    };
    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state
    VkPipelineColorBlendAttachmentState attachmentStates{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1,
    };

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings{
            .binding = 0,
            .stride = 3 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription vertex_input_attributes[] = {
            {
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = 0,
            }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_input_bindings,
            .vertexAttributeDescriptionCount = 1,
            .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    // Create the pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,  // reserved, must be 0
            .initialDataSize = 0,
            .pInitialData = nullptr,
    };

    CALL_VK(vkCreatePipelineCache(device.vk_device, &pipelineCacheInfo, nullptr, &gfxPipeline.vk_pipeline_cache));

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = nullptr,
            .layout = gfxPipeline.vk_pipeline_layout,
            .renderPass = render.vk_render_pass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };

    VkResult pipelineResult = vkCreateGraphicsPipelines(
            device.vk_device, gfxPipeline.vk_pipeline_cache, 1, &pipelineCreateInfo, nullptr,
            &gfxPipeline.vk_pipeline);

    // We don't need the shaders anymore, we can release their memory
    vkDestroyShaderModule(device.vk_device, vertexShader, nullptr);
    vkDestroyShaderModule(device.vk_device, fragmentShader, nullptr);

    return pipelineResult;
}

void delete_graphics_pipeline(void) {
    if (gfxPipeline.vk_pipeline == VK_NULL_HANDLE) return;
    vkDestroyPipeline(device.vk_device, gfxPipeline.vk_pipeline, nullptr);
    vkDestroyPipelineCache(device.vk_device, gfxPipeline.vk_pipeline_cache, nullptr);
    vkDestroyPipelineLayout(device.vk_device, gfxPipeline.vk_pipeline_layout, nullptr);
}

// init_window:
//   Initialize Vulkan Context when android application window is created
//   upon return, vulkan is ready to draw frames
bool init_window(android_app *app) {
    androidAppCtx = app;

    if (!load_vulkan_symbols()) {
        LOGW("Vulkan is unavailable, install vulkan and re-start");
        return false;
    }

    VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "vulkan-example",
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
            .pEngineName = "vulkan-example",
            .engineVersion = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };

    create_vulkan_device(app->window, &app_info);

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

    VkAttachmentReference colourReference = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpassDescription{
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colourReference,
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr,
    };

    VkRenderPassCreateInfo renderPassCreateInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .attachmentCount = 1,
            .pAttachments = &vk_attachment_description,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 0,
            .pDependencies = nullptr,
    };
    CALL_VK(vkCreateRenderPass(device.vk_device, &renderPassCreateInfo, nullptr, &render.vk_render_pass));

    create_frame_buffers(render.vk_render_pass);

    CreateBuffers();

    CreateGraphicsPipeline();

    // -----------------------------------------------
    // Create a pool of command buffers to allocate command buffer from
    VkCommandPoolCreateInfo cmdPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = device.queueFamilyIndex_,
    };
    CALL_VK(vkCreateCommandPool(device.vk_device, &cmdPoolCreateInfo, nullptr, &render.vk_command_pool));

    // Record a command buffer that just clear the screen
    // 1 command buffer draw in 1 framebuffer
    // In our case we need 2 command as we have 2 framebuffer
    render.command_buffer_len = swapchain.swap_chain_length;
    render.vk_command_buffer = new VkCommandBuffer[swapchain.swap_chain_length];
    VkCommandBufferAllocateInfo cmdBufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = render.vk_command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = render.command_buffer_len,
    };
    CALL_VK(vkAllocateCommandBuffers(device.vk_device, &cmdBufferCreateInfo, render.vk_command_buffer));

    for (int bufferIndex = 0; bufferIndex < swapchain.swap_chain_length; bufferIndex++) {
        // We start by creating and declare the "beginning" our command buffer
        VkCommandBufferBeginInfo cmdBufferBeginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = 0,
                .pInheritanceInfo = nullptr,
        };
        CALL_VK(vkBeginCommandBuffer(render.vk_command_buffer[bufferIndex], &cmdBufferBeginInfo));
        // transition the display image to color attachment layout
        set_image_layout(render.vk_command_buffer[bufferIndex],
                         swapchain.vk_images[bufferIndex],
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        // Now we start a renderpass. Any draw command has to be recorded in a
        // renderpass
        VkClearValue clearVals = {.color {.float32 {0.0f, 0.34f, 0.90f, 1.0f}}};
        VkRenderPassBeginInfo renderPassBeginInfo{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = nullptr,
                .renderPass = render.vk_render_pass,
                .framebuffer = swapchain.vk_framebuffers[bufferIndex],
                .renderArea = {.offset {.x = 0, .y = 0,},
                        .extent = swapchain.vk_extend_2d},
                .clearValueCount = 1,
                .pClearValues = &clearVals};
        vkCmdBeginRenderPass(render.vk_command_buffer[bufferIndex], &renderPassBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        // Bind what is necessary to the command buffer
        vkCmdBindPipeline(render.vk_command_buffer[bufferIndex],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, gfxPipeline.vk_pipeline);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(render.vk_command_buffer[bufferIndex], 0, 1, &buffers.vk_buffer, &offset);

        // Draw Triangle
        vkCmdDraw(render.vk_command_buffer[bufferIndex], 3, 1, 0, 0);

        vkCmdEndRenderPass(render.vk_command_buffer[bufferIndex]);

        CALL_VK(vkEndCommandBuffer(render.vk_command_buffer[bufferIndex]));
    }

    // We need to create a fence to be able, in the main loop, to wait for our
    // draw command(s) to finish before swapping the framebuffers
    VkFenceCreateInfo fenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    CALL_VK(vkCreateFence(device.vk_device, &fenceCreateInfo, nullptr, &render.vk_fence));

    // We need to create a semaphore to be able to wait, in the main loop, for our
    // framebuffer to be available for us before drawing.
    VkSemaphoreCreateInfo semaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    CALL_VK(vkCreateSemaphore(device.vk_device, &semaphoreCreateInfo, nullptr, &render.vk_semaphore));

    device.initialized_ = true;
    return true;
}

bool is_vulkan_ready() {
    return device.initialized_;
}

void terminate_window() {
    vkFreeCommandBuffers(device.vk_device, render.vk_command_pool, render.command_buffer_len, render.vk_command_buffer);
    delete[] render.vk_command_buffer;

    vkDestroyCommandPool(device.vk_device, render.vk_command_pool, nullptr);
    vkDestroyRenderPass(device.vk_device, render.vk_render_pass, nullptr);
    delete_swap_chain();
    delete_graphics_pipeline();
    delete_buffers();

    vkDestroyDevice(device.vk_device, nullptr);
    vkDestroyInstance(device.vk_instance, nullptr);

    device.initialized_ = false;
}

bool draw_frame() {
    CALL_VK(vkWaitForFences(device.vk_device, 1, &render.vk_fence, VK_TRUE, 100000000));
    CALL_VK(vkResetFences(device.vk_device, 1, &render.vk_fence));

    uint32_t nextIndex;
    CALL_VK(vkAcquireNextImageKHR(device.vk_device, swapchain.vk_swapchain, UINT64_MAX, render.vk_semaphore, VK_NULL_HANDLE, &nextIndex));

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render.vk_semaphore,
            .pWaitDstStageMask = &waitStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &render.vk_command_buffer[nextIndex],
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
    };
    CALL_VK(vkQueueSubmit(device.vk_queue, 1, &submit_info, render.vk_fence));

    VkResult result;
    VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .swapchainCount = 1,
            .pSwapchains = &swapchain.vk_swapchain,
            .pImageIndices = &nextIndex,
            .pResults = &result,
    };
    vkQueuePresentKHR(device.vk_queue, &presentInfo);
    return true;
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
    VkImageMemoryBarrier imageMemoryBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
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
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;
        default:
            break;
    }

    switch (newImageLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;
        default:
            break;
    }

    vkCmdPipelineBarrier(cmdBuffer, srcStages, destStages, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}
