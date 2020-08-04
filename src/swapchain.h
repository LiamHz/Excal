#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "structs.h"
#include "image.h"

namespace Excal::Swapchain
{
struct SwapchainState {
  vk::SwapchainKHR           swapchain;
  vk::Format                 swapchainImageFormat;
  vk::Extent2D               swapchainExtent;
  //std::vector<vk::Image>     swapchainImages;
  //std::vector<vk::ImageView> swapchainImageViews;
};

SwapchainState createSwapchain(
  const vk::PhysicalDevice&,
  const vk::Device&,
  const vk::SurfaceKHR&,
  GLFWwindow*,
  const QueueFamilyIndices&
);

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>&);
vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR&, GLFWwindow*);
vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
  const std::vector<vk::SurfaceFormatKHR>&
);

void cleanupSwapchain(
  const vk::Device&               device,
  VmaAllocator&                   allocator,
  const vk::CommandPool&          commandPool,
  vk::DescriptorPool&             descriptorPool,
  std::vector<vk::CommandBuffer>& commandBuffers,
  vk::SwapchainKHR&               swapchain,
  std::vector<vk::ImageView>&     swapchainImageViews,
  std::vector<VkFramebuffer>&     swapchainFramebuffers,
  Excal::Image::ImageResources&   colorResources,
  Excal::Image::ImageResources&   depthResources,
  std::vector<vk::Buffer>&        uniformBuffers,
  std::vector<VmaAllocation>&     uniformBufferAllocations,
  vk::RenderPass&                 renderPass,
  vk::Pipeline&                   graphicsPipeline,
  vk::PipelineCache&              pipelineCache,
  vk::PipelineLayout&             pipelineLayout
);

void recreateSwapchain(
  GLFWwindow*                     window,
  vk::DescriptorPool&             descriptorPool,
  std::vector<vk::CommandBuffer>& commandBuffers,
  vk::SwapchainKHR&               swapchain,
  vk::Format&                     swapchainImageFormat,
  vk::Extent2D                    swapchainExtent,
  std::vector<vk::Image>&         swapchainImages,
  std::vector<vk::ImageView>&     swapchainImageViews,
  std::vector<VkFramebuffer>&     swapchainFramebuffers,
  Excal::Image::ImageResources&   colorResources,
  Excal::Image::ImageResources&   depthResources,
  std::vector<vk::Buffer>&        uniformBuffers,
  std::vector<VmaAllocation>&     uniformBufferAllocations,
  vk::RenderPass&                 renderPass,
  vk::Pipeline&                   graphicsPipeline,
  vk::PipelineLayout&             pipelineLayout,
  vk::PipelineCache&              pipelineCache,
  std::vector<vk::DescriptorSet>& descriptorSets,
  const vk::Device&               device,
  const vk::PhysicalDevice&       physicalDevice,
  VmaAllocator&                   allocator,
  const vk::SurfaceKHR&           surface,
  const vk::SampleCountFlagBits&  msaaSamples,
  const vk::Format&               depthFormat,
  const vk::CommandPool&          commandPool,
  const std::vector<uint32_t>&    indexCounts,
  const vk::Buffer&               indexBuffer,
  const vk::Buffer&               vertexBuffer,
  const vk::DescriptorSetLayout&  descriptorSetLayout,
  const vk::Sampler&              textureSampler,
  const vk::ImageView&            textureImageView
);
}
