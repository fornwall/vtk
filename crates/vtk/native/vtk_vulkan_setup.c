#include "vtk_log.h"
#include "vtk_cffi.h"
#include "vtk_array.h"
#include "vtk_vulkan_setup.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
 * set_image_layout():
 *    Helper function to transition color buffer layout
 */
void set_image_layout(VkCommandBuffer cmdBuffer, VkImage image,
                      VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags destStages);

void vtk_create_swap_chain(struct VtkWindow* vtk_window) {
    struct VtkDevice* vtk_device = vtk_window->vtk_device;

    memset(&vtk_window->vk_swapchain, 0, sizeof(vtk_window->vk_swapchain));

    VkSurfaceCapabilitiesKHR vk_surface_capabilities;
    CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vtk_device->vk_physical_device, vtk_window->vk_surface, &vk_surface_capabilities))

    uint32_t format_count = 0;
    CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(vtk_device->vk_physical_device, vtk_window->vk_surface, &format_count, NULL))
    VkSurfaceFormatKHR *formats = VTK_ARRAY_ALLOC(VkSurfaceFormatKHR, format_count);
    CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(device.vk_physical_device, device.vk_surface, &format_count, formats))

    uint32_t chosenFormat;
    for (chosenFormat = 0; chosenFormat < format_count; chosenFormat++) {
        if (formats[chosenFormat].format == VK_FORMAT_B8G8R8A8_SRGB) break;
    }
    assert(chosenFormat < format_count);

    vtk_window->vk_swapchain.vk_extend_2d = vk_surface_capabilities.currentExtent;
    vtk_window->vk_swapchain.vk_format = formats[chosenFormat].format;

    VkSurfaceCapabilitiesKHR surfaceCap;
    CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vtk_device->vk_physical_device, vtk_window->vk_surface, &surfaceCap))
    assert(surfaceCap.supportedCompositeAlpha | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);

    LOGI("vk_surface_capabilities.minImageCount = %d", vk_surface_capabilities.minImageCount);
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

    CALL_VK(vkCreateSwapchainKHR(vtk_device->vk_device, &swapchainCreateInfo, NULL, &vtk_window->vk_swapchain))

    CALL_VK(vkGetSwapchainImagesKHR(vtk_device->vk_device, vtk_window->vk_swapchain, &vtk_window->vk_swapchain.num_images, NULL))

    vtk_window->vk_swapchain.vk_images = VTK_ARRAY_ALLOC(VkImage, vtk_window->vk_swapchain.num_images);
    CALL_VK(vkGetSwapchainImagesKHR(vtk_device->vk_device, vtk_window->vk_swapchain, &vtk_window->vk_swapchain.num_images, vtk_window->vk_swapchain.vk_images))

    vtk_window->vk_swapchain.vk_image_views = VTK_ARRAY_ALLOC(VkImageView, vtk_window->vk_swapchain.num_images)
    for (uint32_t i = 0; i < vtk_window->vk_swapchain.num_images; i++) {
        VkImageViewCreateInfo vk_image_view_create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .image = vtk_window->vk_swapchain.vk_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = vtk_window->vk_swapchain.vk_format,
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
        CALL_VK(vkCreateImageView(vtk_device->vk_device, &vk_image_view_create_info, NULL, &vtk_window->vk_swapchain.vk_image_views[i]))
    }

    free(formats);
}

void vtk_delete_swap_chain(struct VtkWindow* vtk_window) {
    struct VtkDevice* vtk_device = vtk_window->vtk_device;

    for (uint32_t i = 0; i < vtk_window->vk_swapchain.num_images; i++) {
        vkDestroyFramebuffer(vtk_device->vk_device, vtk_window->vk_swapchain.vk_framebuffers[i], NULL);
        vkDestroyImageView(vtk_device->vk_device, vtk_window->vk_swapchain.vk_image_views[i], NULL);
        // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/2718
        // TODO: Delete not presentable image here?
        // vkDestroyImage(device.vk_device, swapchain.vk_images[i], NULL);
    }
    vkDestroySwapchainKHR(vtk_device->vk_device, vtk_window->vk_swapchain.vk_swapchain, NULL);

    free(vtk_window->vk_swapchain.vk_framebuffers);
    free(vtk_window->vk_swapchain.vk_image_views);
    free(vtk_window->vk_swapchain.vk_images);

    vtk_window->vk_swapchain.vk_framebuffers = NULL;
    vtk_window->vk_swapchain.vk_image_views = NULL;
    vtk_window->vk_swapchain.vk_images = NULL;
    // Note that vk_swapchain is just a handle, not a pointer.
}

void vtk_create_frame_buffers(VkRenderPass vk_render_pass) {
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
bool MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
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
    const float vertex_position_data[] = {
            // position[0]
            -1.0f, -1.0f, 0.0f,
            // position[1]
            1.0f, -1.0f, 0.0f,
            // position[2]
            0.0f, 1.0f, 0.0f,
    };
    const float vertex_color_data[] = {
            1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,
    };

    VkBuffer* new_buffers[2] = {
            &buffers.vk_vertex_position_buffer,
            &buffers.vk_vertex_color_buffer,
    };

    VkDeviceMemory deviceMemory;
    void *data;

    for (int i = 0; i < 2; i++) {
        // Create a vertex buffer
        VkBufferCreateInfo createBufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .size = sizeof(vertex_position_data), // TODO: Using same for color and position
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &device.queueFamilyIndex_,
        };

        CALL_VK(vkCreateBuffer(device.vk_device, &createBufferInfo, NULL, new_buffers[i]))

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device.vk_device, *new_buffers[i], &memReq);
        if (i == 0) {
            VkMemoryAllocateInfo vk_memory_allocation_info = {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .pNext = NULL,
                    .allocationSize = memReq.size * 10, // TODO: hack, should allocate more
                    .memoryTypeIndex = 0,  // Memory type assigned in the next step
            };

            // Assign the proper memory type for that buffer
            MapMemoryTypeToIndex(memReq.memoryTypeBits,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 &vk_memory_allocation_info.memoryTypeIndex);

            // Allocate memory for the buffer
            CALL_VK(vkAllocateMemory(device.vk_device, &vk_memory_allocation_info, NULL, &deviceMemory))
            CALL_VK(vkMapMemory(device.vk_device, deviceMemory, 0, vk_memory_allocation_info.allocationSize, 0, &data))
        }

        size_t buffer_offset = ((i == 0) ? 0 : sizeof(vertex_position_data));
        CALL_VK(vkBindBufferMemory(device.vk_device, *new_buffers[i], deviceMemory, buffer_offset))
        memcpy(data + buffer_offset, (i == 0) ? vertex_position_data : vertex_color_data,
               sizeof(vertex_position_data)); // TODO: using position for size for both
    }

    vkUnmapMemory(device.vk_device, deviceMemory);
}

void delete_vertex_buffers(void) {
    vkDestroyBuffer(device.vk_device, buffers.vk_vertex_position_buffer, NULL);
    vkDestroyBuffer(device.vk_device, buffers.vk_vertex_color_buffer, NULL);
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
#else
#define MAXBUFLEN 100000
    char source[MAXBUFLEN + 1];
    FILE *fp = fopen(filePath, "r");
    assert(fp != NULL);
    size_t newLen = fread(source, sizeof(char), MAXBUFLEN, fp);
    assert(ferror(fp) == 0);
    source[newLen] = '\0';
    VkShaderModuleCreateInfo vk_shader_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .codeSize = newLen,
            .pCode = (const uint32_t *) source,
    };
    CALL_VK(vkCreateShaderModule(device.vk_device, &vk_shader_module_create_info, NULL, shaderOut));
    fclose(fp);
#endif
}

VkShaderModule compile_shader(VkDevice a_device, uint8_t *bytes, size_t size) {
    VkShaderModuleCreateInfo vk_shader_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .codeSize = size,
            .pCode = (const uint32_t *) bytes,
    };
    VkShaderModule result;
    CALL_VK(vkCreateShaderModule(a_device, &vk_shader_module_create_info, NULL, &result));
    return result;
}

void vtk_create_graphics_pipeline() {
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
    load_shader_from_file("out/shaders/triangle.vert.spv", &vertexShader);
    load_shader_from_file("out/shaders/triangle.frag.spv", &fragmentShader);

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
    VkVertexInputBindingDescription vk_vertex_input_binding_descriptions[] = {
            {
                    .binding = 0,
                    .stride = 3 * sizeof(float),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
            {
                    .binding = 1,
                    .stride = 3 * sizeof(float),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            }
    };

    VkVertexInputAttributeDescription vk_vertex_input_attribute_descriptions[] = {
            {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = 0,
            },
            {
                    .binding = 1,
                    .location = 1,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = 0,
            },
    };

    VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = NULL,
            .vertexBindingDescriptionCount = VK_WRAP_ARRAY_SIZE(vk_vertex_input_binding_descriptions),
            .pVertexBindingDescriptions = vk_vertex_input_binding_descriptions,
            .vertexAttributeDescriptionCount = VK_WRAP_ARRAY_SIZE(vk_vertex_input_attribute_descriptions),
            .pVertexAttributeDescriptions = vk_vertex_input_attribute_descriptions,
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .stageCount = 2,
            .pStages = shader_stages,
            .pVertexInputState = &vk_pipeline_vertex_input_state_create_info,
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

    // VkPipelineCacheCreateInfo pipelineCacheInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, .pNext = NULL, .flags = 0,  // reserved, must be 0 .initialDataSize = 0, .pInitialData = NULL, };
    //CALL_VK(vkCreatePipelineCache(device.vk_device, &pipelineCacheInfo, NULL, &gfxPipeline.vk_pipeline_cache));
    CALL_VK(vkCreateGraphicsPipelines(device.vk_device, NULL /*gfxPipeline.vk_pipeline_cache*/, 1, &pipelineCreateInfo, NULL,
                                      &gfxPipeline.vk_pipeline))

    // We don't need the shaders anymore, we can release their memory
    vkDestroyShaderModule(device.vk_device, vertexShader, NULL);
    vkDestroyShaderModule(device.vk_device, fragmentShader, NULL);
}

void vtk_create_command_buffers(struct VtkWindow* vtk_window) {
    // Record a command buffer that just clear the screen
    // 1 command buffer draw in 1 framebuffer
    // In our case we create one command buffer per swap chain image.
    render.command_buffer_len = swapchain.num_images;
    render.vk_command_buffers = VK_WRAP_ALLOC_ARRAY(VkCommandBuffer, swapchain.num_images);
    VkCommandBufferAllocateInfo vk_command_buffers_allocate_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = render.vk_command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = render.command_buffer_len,
    };
    CALL_VK(vkAllocateCommandBuffers(device.vk_device, &vk_command_buffers_allocate_info, render.vk_command_buffers));

    for (uint32_t bufferIndex = 0; bufferIndex < swapchain.num_images; bufferIndex++) {
        // We start by creating and declare the "beginning" our command buffer
        VkCommandBufferBeginInfo vk_command_buffers_begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = NULL,
                .flags = 0,
                .pInheritanceInfo = NULL,
        };
        CALL_VK(vkBeginCommandBuffer(render.vk_command_buffers[bufferIndex], &vk_command_buffers_begin_info));
        // transition the display image to color attachment layout
        set_image_layout(render.vk_command_buffers[bufferIndex],
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

        vkCmdBeginRenderPass(render.vk_command_buffers[bufferIndex], &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(render.vk_command_buffers[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, gfxPipeline.vk_pipeline);

            uint32_t first_binding = 0;
            VkBuffer new_buffers[2] = {buffers.vk_vertex_position_buffer, buffers.vk_vertex_color_buffer};
            uint32_t binding_count = 2;
            VkDeviceSize buffer_offsets[2] = {0, 0};
            vkCmdBindVertexBuffers(render.vk_command_buffers[bufferIndex], first_binding, binding_count, new_buffers,
                                   buffer_offsets);

            uint32_t vertex_count = 3;
            uint32_t instance_count = 1;
            uint32_t first_vertex = 0;
            uint32_t first_instance = 0;
            vkCmdDraw(render.vk_command_buffers[bufferIndex], vertex_count, instance_count, first_vertex, first_instance);
        }
        vkCmdEndRenderPass(render.vk_command_buffers[bufferIndex]);
        CALL_VK(vkEndCommandBuffer(render.vk_command_buffers[bufferIndex]));
    }
}

void delete_graphics_pipeline(void) {
    if (gfxPipeline.vk_pipeline == VK_NULL_HANDLE) return;
    vkDestroyPipeline(device.vk_device, gfxPipeline.vk_pipeline, NULL);
    //vkDestroyPipelineCache(device.vk_device, gfxPipeline.vk_pipeline_cache, NULL);
    vkDestroyPipelineLayout(device.vk_device, gfxPipeline.vk_pipeline_layout, NULL);
    // TODO: free memory?
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

    create_command_buffers();

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

void delete_command_buffers() {
    vkFreeCommandBuffers(device.vk_device, render.vk_command_pool, render.command_buffer_len, render.vk_command_buffers);
    free(render.vk_command_buffers);
}

void terminate_window() {
    delete_command_buffers();
    vkDestroyCommandPool(device.vk_device, render.vk_command_pool, NULL);
    vkDestroyRenderPass(device.vk_device, render.vk_render_pass, NULL);
    delete_swap_chain();
    delete_graphics_pipeline();
    delete_vertex_buffers();

    vkDestroyDevice(device.vk_device, NULL);
    vkDestroyInstance(device.vk_instance, NULL);

    device.initialized_ = false;
}

void recreate_swap_chain() {
    CALL_VK(vkDeviceWaitIdle(device.vk_device))

    delete_swap_chain();
    delete_graphics_pipeline();
    delete_command_buffers();

    create_swap_chain();
    create_frame_buffers(render.vk_render_pass);
    create_graphics_pipeline();
    create_command_buffers();
}

void draw_frame() {
    CALL_VK(vkWaitForFences(device.vk_device, 1, &render.vk_fence, VK_TRUE, UINT64_MAX))

    uint32_t acquired_image_idx;
    VkResult acquire_result = vkAcquireNextImageKHR(device.vk_device, swapchain.vk_swapchain, UINT64_MAX, render.vk_semaphore,
                                                    VK_NULL_HANDLE, &acquired_image_idx);
    switch (acquire_result) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            LOGI("vkAcquireNextImageKHR() returned VK_ERROR_OUT_OF_DATE_KHR - recreating... %d", 1);
            // We cannot present it - recreate and return.
            recreate_swap_chain();
            break;
        case VK_SUBOPTIMAL_KHR:
            // Ok to go ahead and present image - recreate after present.
            LOGI("vkAcquireNextImageKHR() returned VK_SUBOPTIMAL_KHR, %d", 1);
            break;
        default:
            LOGE("vkAcquireNextImageKHR failed");
            assert(false);
            break;
    }

    CALL_VK(vkResetFences(device.vk_device, 1, &render.vk_fence))

    VkPipelineStageFlags vk_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render.vk_semaphore,
            .pWaitDstStageMask = &vk_pipeline_stage_flags,
            .commandBufferCount = 1,
            .pCommandBuffers = &render.vk_command_buffers[acquired_image_idx],
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
    VkResult present_result = vkQueuePresentKHR(device.vk_queue, &presentInfo);
    switch (present_result) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            LOGI("vkQueuePresentKHR() returned VK_SUBOPTIMAL_KHR | VK_ERROR_OUT_OF_DATE_KHR - recreating...");
            recreate_swap_chain();
            break;
        default:
            LOGE("vkQueuePresentKHR failed");
            assert(false);
            break;
    }
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
