#include "vtk_array.h"
#include "vtk_cffi.h"
#include "vtk_internal.h"
#include "vtk_log.h"
#include "vtk_platform.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
 * set_image_layout():
 *    Helper function to transition color buffer layout
 */
void set_image_layout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldImageLayout,
                      VkImageLayout newImageLayout, VkPipelineStageFlags srcStages, VkPipelineStageFlags destStages);

void vtk_setup_surface_format(struct VtkWindowNative *vtk_window) {
  assert(vtk_window != NULL);
  assert(vtk_window->vtk_device != NULL);
  assert(vtk_window->vk_surface != NULL);
  assert(vtk_window->vtk_device->vk_physical_device != NULL);
  LOGI("Before vkGetPhysicalDeviceSurfaceFormatsKHR");
  uint32_t format_count = 0;
  CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(vtk_window->vtk_device->vk_physical_device, vtk_window->vk_surface,
                                               &format_count, NULL))
  LOGI("After vkGetPhysicalDeviceSurfaceFormatsKHR");
  VkSurfaceFormatKHR *formats = VTK_ARRAY_ALLOC(VkSurfaceFormatKHR, format_count);
  CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(vtk_window->vtk_device->vk_physical_device, vtk_window->vk_surface,
                                               &format_count, formats))
  uint32_t chosenFormat;
  for (chosenFormat = 0; chosenFormat < format_count; chosenFormat++) {
    if (formats[chosenFormat].format == VK_FORMAT_B8G8R8A8_SRGB)
      break;
  }
  assert(chosenFormat < format_count);
  vtk_window->vk_surface_format = formats[chosenFormat].format;
  vtk_window->vk_color_space = formats[chosenFormat].colorSpace;
  free(formats);
}

void vtk_create_swap_chain(struct VtkWindowNative *vtk_window) {
  struct VtkDeviceNative *vtk_device = vtk_window->vtk_device;

  memset(&vtk_window->vk_swapchain, 0, sizeof(vtk_window->vk_swapchain));

  VkSurfaceCapabilitiesKHR vk_surface_capabilities;
  CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vtk_device->vk_physical_device, vtk_window->vk_surface,
                                                    &vk_surface_capabilities))
#ifndef VTK_PLATFORM_WAYLAND
  // On Wayland we need to set the surface width here when creating the swap chain.
  vtk_window->vk_extent_2d = vk_surface_capabilities.currentExtent;
#endif

  // VkSurfaceCapabilitiesKHR surfaceCap;
  // CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vtk_device->vk_physical_device, vtk_window->vk_surface,
  // &surfaceCap)) assert(surfaceCap.supportedCompositeAlpha | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);

  LOGI("vk_surface_capabilities.minImageCount = %d", vk_surface_capabilities.minImageCount);
  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = NULL,
      .surface = vtk_window->vk_surface,
      .minImageCount = vk_surface_capabilities.minImageCount,
      .imageFormat = vtk_window->vk_surface_format,
      .imageColorSpace = vtk_window->vk_color_space,
      .imageExtent = vtk_window->vk_extent_2d,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &vtk_device->graphics_queue_family_idx,
      // Handle rotation ourselves for best performance, see:
      // https://developer.android.com/games/optimize/vulkan-prerotation
      .preTransform = vk_surface_capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
  };

  CALL_VK(vkCreateSwapchainKHR(vtk_device->vk_device, &swapchainCreateInfo, NULL, &vtk_window->vk_swapchain))

  uint32_t num_images;
  CALL_VK(vkGetSwapchainImagesKHR(vtk_device->vk_device, vtk_window->vk_swapchain, &num_images, NULL))
  vtk_window->num_swap_chain_images = (uint8_t)num_images;

  vtk_window->vk_swap_chain_images = VTK_ARRAY_ALLOC(VkImage, num_images);
  CALL_VK(vkGetSwapchainImagesKHR(vtk_device->vk_device, vtk_window->vk_swapchain, &num_images,
                                  vtk_window->vk_swap_chain_images))

  VkImageView depth_view = VK_NULL_HANDLE;

  vtk_window->vk_swap_chain_images_views = VTK_ARRAY_ALLOC(VkImageView, num_images);
  vtk_window->vk_swap_chain_framebuffers = VTK_ARRAY_ALLOC(VkFramebuffer, num_images);

  for (uint32_t i = 0; i < num_images; i++) {
    VkImageViewCreateInfo vk_image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = vtk_window->vk_swap_chain_images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vtk_window->vk_surface_format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    CALL_VK(vkCreateImageView(vtk_device->vk_device, &vk_image_view_create_info, NULL,
                              &vtk_window->vk_swap_chain_images_views[i]))

    VkImageView attachments[2] = {
        vtk_window->vk_swap_chain_images_views[i],
        depth_view,
    };
    VkFramebufferCreateInfo vk_frame_buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = vtk_window->vk_surface_render_pass,
        .attachmentCount = (uint32_t)((depth_view == VK_NULL_HANDLE ? 1 : 2)),
        .pAttachments = attachments,
        .width = (uint32_t)vtk_window->vk_extent_2d.width,
        .height = (uint32_t)vtk_window->vk_extent_2d.height,
        .layers = 1,
    };
    CALL_VK(vkCreateFramebuffer(vtk_window->vtk_device->vk_device, &vk_frame_buffer_create_info, NULL,
                                &vtk_window->vk_swap_chain_framebuffers[i]));
  }
}

void vtk_delete_swap_chain(struct VtkWindowNative *vtk_window) {
  struct VtkDeviceNative *vtk_device = vtk_window->vtk_device;

  for (uint32_t i = 0; i < vtk_window->num_swap_chain_images; i++) {
    vkDestroyFramebuffer(vtk_device->vk_device, vtk_window->vk_swap_chain_framebuffers[i], NULL);
    vkDestroyImageView(vtk_device->vk_device, vtk_window->vk_swap_chain_images_views[i], NULL);
    // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/2718
    // TODO: Delete not presentable image here?
    // vkDestroyImage(device.vk_device, swapchain.vk_images[i], NULL);
  }
  vkDestroySwapchainKHR(vtk_device->vk_device, vtk_window->vk_swapchain, NULL);

  free(vtk_window->vk_swap_chain_framebuffers);
  free(vtk_window->vk_swap_chain_images_views);
  free(vtk_window->vk_swap_chain_images);

  // vtk_window->vk_swapchain.vk_framebuffers = NULL;
  // vtk_window->vk_swapchain.vk_image_views = NULL;
  // vtk_window->vk_swapchain.vk_images = NULL;
  //  Note that vk_swapchain is just a handle, not a pointer.
}

void vtk_create_surface_render_pass(struct VtkWindowNative *vtk_window) {
  VkAttachmentDescription vk_color_attachment_description = {
      .format = vtk_window->vk_surface_format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference vk_color_attachment_reference = {.attachment = 0,
                                                         .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription vk_depth_attachment_description = {
      .format = vtk_window->vk_surface_format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkSubpassDescription vk_subpass_description = {
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = NULL,
      .colorAttachmentCount = 1,
      .pColorAttachments = &vk_color_attachment_reference,
      .pResolveAttachments = NULL,
      .pDepthStencilAttachment = NULL,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = NULL,
  };

  VkRenderPassCreateInfo vk_render_pass_create_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = NULL,
      .attachmentCount = 1,
      .pAttachments = &vk_color_attachment_description,
      .subpassCount = 1,
      .pSubpasses = &vk_subpass_description,
      .dependencyCount = 0,
      .pDependencies = NULL,
  };
  CALL_VK(vkCreateRenderPass(vtk_window->vtk_device->vk_device, &vk_render_pass_create_info, NULL,
                             &vtk_window->vk_surface_render_pass))
}

void vtk_create_graphics_pipeline(struct VtkWindowNative *vtk_window, uint32_t push_constant_size) {
  VkPushConstantRange vk_push_constant_range = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = push_constant_size,
  };

  VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = NULL,
      .setLayoutCount = 0,
      .pSetLayouts = NULL,
      .pushConstantRangeCount = (uint32_t)((push_constant_size == 0) ? 0 : 1),
      .pPushConstantRanges = &vk_push_constant_range,
  };
  CALL_VK(vkCreatePipelineLayout(vtk_window->vtk_device->vk_device, &vk_pipeline_layout_create_info, NULL,
                                 &vtk_window->vk_pipeline_layout));

  VkShaderModule vertexShader, fragmentShader;
  // load_shader_from_file("out/shaders/triangle.vert.spv", &vertexShader);
  // load_shader_from_file("out/shaders/triangle.frag.spv", &fragmentShader);

  // Specify vertex and fragment shader stages
  /*
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
          */

  VkViewport viewports = {
      .x = 0,
      .y = 0,
      .width = (float)vtk_window->vk_extent_2d.width,
      .height = (float)vtk_window->vk_extent_2d.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  VkRect2D scissor = {
      .offset = {.x = 0, .y = 0},
      .extent = vtk_window->vk_extent_2d,
  };

  VkPipelineViewportStateCreateInfo viewportInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = NULL,
      .viewportCount = 1,
      .pViewports = &viewports,
      .scissorCount = 1,
      .pScissors = &scissor,
  };

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
      .colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
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

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = NULL,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

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
      }};

  VkVertexInputAttributeDescription vk_vertex_input_attribute_descriptions[] = {
      {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0,
      },
      {
          .location = 1,
          .binding = 1,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0,
      },
  };

  VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = NULL,
      .vertexBindingDescriptionCount = VTK_ARRAY_SIZE(vk_vertex_input_binding_descriptions),
      .pVertexBindingDescriptions = vk_vertex_input_binding_descriptions,
      .vertexAttributeDescriptionCount = VTK_ARRAY_SIZE(vk_vertex_input_attribute_descriptions),
      .pVertexAttributeDescriptions = vk_vertex_input_attribute_descriptions,
  };

  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .stageCount = 0,
      .pStages = NULL, // shader_stages,
      .pVertexInputState = &vk_pipeline_vertex_input_state_create_info,
      .pInputAssemblyState = &inputAssemblyInfo,
      .pTessellationState = NULL,
      .pViewportState = &viewportInfo,
      .pRasterizationState = &rasterInfo,
      .pMultisampleState = &multisampleInfo,
      .pDepthStencilState = NULL,
      .pColorBlendState = &colorBlendInfo,
      .pDynamicState = NULL,
      .layout = vtk_window->vk_pipeline_layout,
      .renderPass = vtk_window->vk_surface_render_pass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
  };

  // VkPipelineCacheCreateInfo pipelineCacheInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, .pNext =
  // NULL, .flags = 0,  // reserved, must be 0 .initialDataSize = 0, .pInitialData = NULL, };
  // CALL_VK(vkCreatePipelineCache(device.vk_device, &pipelineCacheInfo, NULL, &gfxPipeline.vk_pipeline_cache));
  CALL_VK(vkCreateGraphicsPipelines(vtk_window->vtk_device->vk_device, NULL /*gfxPipeline.vk_pipeline_cache*/, 1,
                                    &pipelineCreateInfo, NULL, &vtk_window->vk_pipeline))

  // We don't need the shaders anymore, we can release their memory
  // vkDestroyShaderModule(vtk_window->vtk_device->vk_device, vertexShader, NULL);
  // vkDestroyShaderModule(vtk_window->vtk_device->vk_device, fragmentShader, NULL);
}

void vtk_create_command_buffers(struct VtkWindowNative *vtk_window) {
  // In our case we create one command buffer per swap chain image.
  vtk_window->command_buffer_len = vtk_window->num_swap_chain_images;
  vtk_window->vk_command_buffers = VTK_ARRAY_ALLOC(VkCommandBuffer, vtk_window->num_swap_chain_images);
  VkCommandBufferAllocateInfo vk_command_buffers_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = NULL,
      .commandPool = vtk_window->vtk_device->vk_command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = vtk_window->command_buffer_len,
  };
  CALL_VK(vkAllocateCommandBuffers(vtk_window->vtk_device->vk_device, &vk_command_buffers_allocate_info,
                                   vtk_window->vk_command_buffers));
}

void vtk_record_command_buffers(struct VtkWindowNative *vtk_window) {
  for (uint32_t bufferIndex = 0; bufferIndex < vtk_window->num_swap_chain_images; bufferIndex++) {
    // We start by creating and declare the "beginning" our command buffer
    VkCommandBufferBeginInfo vk_command_buffers_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };
    CALL_VK(vkBeginCommandBuffer(vtk_window->vk_command_buffers[bufferIndex], &vk_command_buffers_begin_info));
    // transition the display image to color attachment layout
    set_image_layout(vtk_window->vk_command_buffers[bufferIndex], vtk_window->vk_swap_chain_images[bufferIndex],
                     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    // Now we start a renderpass. Any draw command has to be recorded in a renderpass.
    VkClearValue vk_clear_value = {.color = {.float32 = {1.0f, 0.0f, 1.0f, 1.0f}}};
    VkRenderPassBeginInfo vk_render_pass_begin_info = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                       .pNext = NULL,
                                                       .renderPass = vtk_window->vk_surface_render_pass,
                                                       .framebuffer =
                                                           vtk_window->vk_swap_chain_framebuffers[bufferIndex],
                                                       .renderArea = {.offset =
                                                                          {
                                                                              .x = 0,
                                                                              .y = 0,
                                                                          },
                                                                      .extent = vtk_window->vk_extent_2d},
                                                       .clearValueCount = 1,
                                                       .pClearValues = &vk_clear_value};

    vkCmdBeginRenderPass(vtk_window->vk_command_buffers[bufferIndex], &vk_render_pass_begin_info,
                         VK_SUBPASS_CONTENTS_INLINE);
    /*
    {
        vkCmdBindPipeline(vtk_window->vk_command_buffers[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
    gfxPipeline.vk_pipeline);

        //vkCmdPushConstants(cmd, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants),
    &constants);

        uint32_t first_binding = 0;
        VkBuffer new_buffers[2] = {buffers.vk_vertex_position_buffer, buffers.vk_vertex_color_buffer};
        uint32_t binding_count = 2;
        VkDeviceSize buffer_offsets[2] = {0, 0};
        vkCmdBindVertexBuffers(vtk_window->vk_command_buffers[bufferIndex], first_binding, binding_count, new_buffers,
    buffer_offsets);

        uint32_t vertex_count = 3;
        uint32_t instance_count = 1;
        uint32_t first_vertex = 0;
        uint32_t first_instance = 0;
        vkCmdDraw(vtk_window->vk_command_buffers[bufferIndex], vertex_count, instance_count, first_vertex,
    first_instance);
    }
    */
    vkCmdEndRenderPass(vtk_window->vk_command_buffers[bufferIndex]);
    CALL_VK(vkEndCommandBuffer(vtk_window->vk_command_buffers[bufferIndex]));
  }
}

void vtk_create_sync(struct VtkWindowNative *vtk_window) {
  // We need to create a fence to be able, in the main loop, to wait for our
  // draw command(s) to finish before swapping the framebuffers
  VkFenceCreateInfo vk_fence_create_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  CALL_VK(vkCreateFence(vtk_window->vtk_device->vk_device, &vk_fence_create_info, NULL, &vtk_window->vk_fence));

  // We need to create a semaphore to be able to wait, in the main loop, for our
  // framebuffer to be available for us before drawing.
  VkSemaphoreCreateInfo vk_semaphore_create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
  };
  CALL_VK(
      vkCreateSemaphore(vtk_window->vtk_device->vk_device, &vk_semaphore_create_info, NULL, &vtk_window->vk_semaphore));
}

void vtk_setup_window_rendering_repeat(struct VtkWindowNative *vtk_window) {
  vtk_create_swap_chain(vtk_window);
  vtk_record_command_buffers(vtk_window);
}

void vtk_setup_window_rendering(struct VtkWindowNative *vtk_window) {
  vtk_create_sync(vtk_window);
  vtk_setup_surface_format(vtk_window);
  vtk_create_surface_render_pass(vtk_window);

  vtk_create_swap_chain(vtk_window);
  vtk_create_command_buffers(vtk_window);
  vtk_record_command_buffers(vtk_window);
}

// set_image_layout():
//    Helper function to transition color buffer layout
void set_image_layout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldImageLayout,
                      VkImageLayout newImageLayout, VkPipelineStageFlags srcStages, VkPipelineStageFlags destStages) {
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
      .subresourceRange =
          {
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

void vtk_terminate_window(struct VtkWindowNative *vtk_window) {
  vkFreeCommandBuffers(vtk_window->vtk_device->vk_device, vtk_window->vtk_device->vk_command_pool,
                       vtk_window->command_buffer_len, vtk_window->vk_command_buffers);
  free(vtk_window->vk_command_buffers);

  vkDestroyRenderPass(vtk_window->vtk_device->vk_device, vtk_window->vk_surface_render_pass, NULL);

  vtk_delete_swap_chain(vtk_window);
  // delete_graphics_pipeline();
  // delete_vertex_buffers();

  // vkDestroyDevice(device.vk_device, NULL);
  // vkDestroyInstance(device.vk_instance, NULL);
  // device.initialized_ = false;
}

void vtk_recreate_swap_chain(struct VtkWindowNative *vtk_window) {
  CALL_VK(vkDeviceWaitIdle(vtk_window->vtk_device->vk_device))

  vtk_delete_swap_chain(vtk_window);
  vtk_setup_window_rendering_repeat(vtk_window);
  // delete_graphics_pipeline();
  // delete_command_buffers();

  // vtk_create_swap_chain();
  // vtk_create_graphics_pipeline();
  // vtk_create_command_buffers();
}

void vtk_render_frame(struct VtkWindowNative *vtk_window) {
  CALL_VK(vkWaitForFences(vtk_window->vtk_device->vk_device, 1, &vtk_window->vk_fence, VK_TRUE, UINT64_MAX))

  uint32_t acquired_image_idx;
  VkResult acquire_result =
      vkAcquireNextImageKHR(vtk_window->vtk_device->vk_device, vtk_window->vk_swapchain, UINT64_MAX,
                            vtk_window->vk_semaphore, VK_NULL_HANDLE, &acquired_image_idx);
  switch (acquire_result) {
  case VK_SUCCESS:
    break;
  case VK_ERROR_OUT_OF_DATE_KHR:
    LOGI("vkAcquireNextImageKHR() returned VK_ERROR_OUT_OF_DATE_KHR - recreating... %d", 1);
    // We cannot present it - recreate and return.
    vtk_recreate_swap_chain(vtk_window);
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

  CALL_VK(vkResetFences(vtk_window->vtk_device->vk_device, 1, &vtk_window->vk_fence))

  VkPipelineStageFlags vk_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                              .pNext = NULL,
                              .waitSemaphoreCount = 1,
                              .pWaitSemaphores = &vtk_window->vk_semaphore,
                              .pWaitDstStageMask = &vk_pipeline_stage_flags,
                              .commandBufferCount = 1,
                              .pCommandBuffers = &vtk_window->vk_command_buffers[acquired_image_idx],
                              .signalSemaphoreCount = 0,
                              .pSignalSemaphores = NULL};
  CALL_VK(vkQueueSubmit(vtk_window->vtk_device->vk_queue, 1, &submit_info, vtk_window->vk_fence))

  VkResult result;
  VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = NULL,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = NULL,
      .swapchainCount = 1,
      .pSwapchains = &vtk_window->vk_swapchain,
      .pImageIndices = &acquired_image_idx,
      .pResults = &result,
  };
  VkResult present_result = vkQueuePresentKHR(vtk_window->vtk_device->vk_queue, &presentInfo);
  switch (present_result) {
  case VK_SUCCESS:
    break;
  case VK_SUBOPTIMAL_KHR:
    LOGI("vkQueuePresentKHR() returned VK_SUBOPTIMAL_KHR - recreating...");
    vtk_recreate_swap_chain(vtk_window);
    break;
  case VK_ERROR_OUT_OF_DATE_KHR:
    LOGI("vkQueuePresentKHR() returned VK_ERROR_OUT_OF_DATE_KHR - recreating...");
    vtk_recreate_swap_chain(vtk_window);
    break;
  default:
    LOGE("vkQueuePresentKHR failed");
    assert(false);
    break;
  }
#ifdef VTK_PLATFORM_WAYLAND
  if (vtk_window->wayland_size_requested_by_compositor.width != vtk_window->vk_extent_2d.width ||
      vtk_window->wayland_size_requested_by_compositor.height != vtk_window->vk_extent_2d.height) {
    vtk_window->vk_extent_2d = vtk_window->wayland_size_requested_by_compositor;
    vtk_recreate_swap_chain(vtk_window);
  }
#endif
}

uint32_t vtk_find_memory_idx(VkPhysicalDevice vk_device, uint32_t typeBits, VkFlags requirements_mask, bool *found) {
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(vk_device, &memoryProperties);
  for (uint32_t memory_index = 0; memory_index < memoryProperties.memoryTypeCount; memory_index++) {
    if ((typeBits & (1 << memory_index)) == 1) {
      if ((memoryProperties.memoryTypes[memory_index].propertyFlags & requirements_mask) == requirements_mask) {
        *found = true;
        return memory_index;
      }
    }
  }
  *found = false;
  return 0;
}

void vtk_create_vertex_buffer(struct VtkDeviceNative *vtk_device, uint64_t buffer_size) {
  VkDevice vk_device = vtk_device->vk_device;
  VkBuffer *vk_vertex_buffer = &vtk_device->vk_vertex_buffer;
  VkDeviceMemory *vk_device_memory = &vtk_device->vk_vertex_buffer_device_memory;

  VkBufferCreateInfo createBufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &vtk_device->graphics_queue_family_idx,
  };
  CALL_VK(vkCreateBuffer(vk_device, &createBufferInfo, NULL, vk_vertex_buffer))

  VkMemoryRequirements vk_memory_requirements;
  vkGetBufferMemoryRequirements(vk_device, *vk_vertex_buffer, &vk_memory_requirements);
  bool found_memory_type;
  VkFlags memory_type_bits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  uint32_t memory_type_idx = vtk_find_memory_idx(vtk_device->vk_physical_device, vk_memory_requirements.memoryTypeBits,
                                                 memory_type_bits, &found_memory_type);
  assert(found_memory_type);
  VkMemoryAllocateInfo vk_memory_allocation_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = NULL,
      .allocationSize = buffer_size,
      .memoryTypeIndex = memory_type_idx,
  };

  CALL_VK(vkAllocateMemory(vk_device, &vk_memory_allocation_info, NULL, vk_device_memory))
  CALL_VK(vkMapMemory(vk_device, *vk_device_memory, 0, vk_memory_allocation_info.allocationSize, 0,
                      &vtk_device->vertex_buffer_ptr))
  CALL_VK(vkBindBufferMemory(vk_device, *vk_vertex_buffer, *vk_device_memory, 0))
  vtk_device->vertex_buffer_size = buffer_size;
}

/*
void delete_vertex_buffers(void) {
    vkDestroyBuffer(device.vk_device, buffers.vk_vertex_position_buffer, NULL);
    vkDestroyBuffer(device.vk_device, buffers.vk_vertex_color_buffer, NULL);
}

void delete_graphics_pipeline(void) {
    if (gfxPipeline.vk_pipeline == VK_NULL_HANDLE) return;
    vkDestroyPipeline(device.vk_device, gfxPipeline.vk_pipeline, NULL);
    //vkDestroyPipelineCache(device.vk_device, gfxPipeline.vk_pipeline_cache, NULL);
    vkDestroyPipelineLayout(device.vk_device, gfxPipeline.vk_pipeline_layout, NULL);
    // TODO: free memory?
}

void terminate_window() {
    delete_swap_chain();
    delete_graphics_pipeline();
    delete_vertex_buffers();

    vkDestroyDevice(device.vk_device, NULL);
    vkDestroyInstance(device.vk_instance, NULL);
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
*/
