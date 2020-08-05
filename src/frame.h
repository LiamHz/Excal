#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "image.h"
#include "structs.h"
#include "model.h"

namespace Excal::Frame
{
void drawFrame(
  // Required for call to Excal::Swapchain::recreateSwapchain()
  // but otherwise not for drawFrame()
  GLFWwindow*                       window,
  vk::DescriptorPool&               descriptorPool,
  std::vector<vk::CommandBuffer>&   commandBuffers,
  vk::SwapchainKHR&                 swapchain,
  vk::Format&                       swapchainImageFormat,
  vk::Extent2D                      swapchainExtent,
  std::vector<vk::Image>&           swapchainImages,
  std::vector<vk::ImageView>&       swapchainImageViews,
  std::vector<VkFramebuffer>&       swapchainFramebuffers,
  Excal::Image::ImageResources&     colorResources,
  Excal::Image::ImageResources&     depthResources,
  std::vector<vk::Buffer>&          uniformBuffers,
  vk::RenderPass&                   renderPass,
  vk::Pipeline&                     graphicsPipeline,
  vk::PipelineLayout&               pipelineLayout,
  vk::PipelineCache&                pipelineCache,
  std::vector<vk::DescriptorSet>&   descriptorSets,
  const vk::PhysicalDevice&         physicalDevice,
  const vk::SurfaceKHR&             surface,
  const vk::SampleCountFlagBits&    msaaSamples,
  const vk::Format&                 depthFormat,
  const vk::CommandPool&            commandPool,
  const std::vector<uint32_t>&      indexCounts,
  const vk::Buffer&                 indexBuffer,
  const vk::Buffer&                 vertexBuffer,
  const vk::DescriptorSetLayout&    descriptorSetLayout,
  const std::vector<vk::ImageView>& textureImageViews,
  const vk::Sampler&                textureSampler,
  std::vector<vk::Buffer>&          dynamicUniformBuffers,

  // Required for regular drawFrame() functionality
  size_t&                           currentFrame,
  bool&                             framebufferResized,
  VmaAllocator&                     allocator,
  std::vector<VmaAllocation>&       uniformBufferAllocations,
  std::vector<VmaAllocation>&       dynamicUniformBufferAllocations,
  const size_t                      dynamicAlignment,
  UboDynamicData&                   uboDynamicData,
  std::vector<vk::Fence>&           imagesInFlight,
  const vk::Device&                 device,
  const vk::Queue&                  graphicsQueue,
  const vk::Queue&                  presentQueue,
  const std::vector<vk::Fence>&     inFlightFences,
  const std::vector<vk::Semaphore>& imageAvailableSemaphores,
  const std::vector<vk::Semaphore>& renderFinishedSemaphores,
  const int                         maxFramesInFlight,
  const std::vector<Excal::Model::Model>& models
);
}
