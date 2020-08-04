#include "frame.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "swapchain.h"
#include "buffer.h"
#include "image.h"

namespace Excal::Frame
{
void drawFrame(
  // Required for call to Excal::Swapchain::recreateSwapchain()
  // but otherwise not for drawFrame()
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
  vk::RenderPass&                 renderPass,
  vk::Pipeline&                   graphicsPipeline,
  vk::PipelineLayout&             pipelineLayout,
  vk::PipelineCache&              pipelineCache,
  std::vector<vk::DescriptorSet>& descriptorSets,
  const vk::PhysicalDevice&       physicalDevice,
  const vk::SurfaceKHR&           surface,
  const vk::SampleCountFlagBits&  msaaSamples,
  const vk::Format&               depthFormat,
  const vk::CommandPool&          commandPool,
  const std::vector<uint32_t>&    indexCounts,
  const vk::Buffer&               indexBuffer,
  const vk::Buffer&               vertexBuffer,
  const vk::DescriptorSetLayout&  descriptorSetLayout,
  const vk::Sampler&              textureSampler,
  const vk::ImageView&            textureImageView,

  // Required for regular drawFrame() functionality
  size_t&                           currentFrame,
  bool&                             framebufferResized,
  VmaAllocator&                     allocator,
  std::vector<VmaAllocation>&       uniformBufferAllocations,
  std::vector<vk::Fence>&           imagesInFlight,
  const vk::Device&                 device,
  const vk::Queue&                  graphicsQueue,
  const vk::Queue&                  presentQueue,
  const std::vector<vk::Fence>&     inFlightFences,
  const std::vector<vk::Semaphore>& imageAvailableSemaphores,
  const std::vector<vk::Semaphore>& renderFinishedSemaphores,
  const int                         maxFramesInFlight
) {
  device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  vk::Result result = device.acquireNextImageKHR(
    swapchain, UINT64_MAX,
    imageAvailableSemaphores[currentFrame],
    nullptr, &imageIndex
  );

  // Usually happens after a window resize
  if (   result == vk::Result::eErrorOutOfDateKHR
      || result == vk::Result::eSuboptimalKHR
  ) {
    Excal::Swapchain::recreateSwapchain(
      window,                   descriptorPool,       commandBuffers,
      swapchain,                swapchainImageFormat, swapchainExtent,
      swapchainImages,          swapchainImageViews,  swapchainFramebuffers,
      colorResources,           depthResources,       uniformBuffers,
      uniformBufferAllocations, renderPass,           graphicsPipeline,
      pipelineLayout,           pipelineCache,        descriptorSets,
      device,                   physicalDevice,       allocator,
      surface,                  msaaSamples,          depthFormat,
      commandPool,              indexCounts,          indexBuffer,
      vertexBuffer,             descriptorSetLayout,  textureSampler,
      textureImageView
    );
    return;
  }

  Excal::Buffer::updateUniformBuffer(
    allocator, uniformBufferAllocations,
    device,    swapchainExtent,
    imageIndex
  );

  // Check if a previous frame is using this image
  // (i.e. there is its fence to wait on)
  if (imagesInFlight[imageIndex]) {
    device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  }
  // Mark the image as now being in use by this frame
  imagesInFlight[imageIndex] = inFlightFences[currentFrame];

  vk::Semaphore signalSemaphores[]    = {renderFinishedSemaphores[currentFrame]};
  vk::Semaphore waitSemaphores[]      = {imageAvailableSemaphores[currentFrame]};
  vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

  vk::SubmitInfo submitInfo(
    1, waitSemaphores, waitStages,
    1, &commandBuffers[imageIndex],
    1, signalSemaphores
  );

  device.resetFences(1, &inFlightFences[currentFrame]);

  graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);

  vk::SwapchainKHR swapchains[] = {swapchain};

  result = presentQueue.presentKHR(
    vk::PresentInfoKHR(1, signalSemaphores, 1, swapchains, &imageIndex)
  );

  if (   result == vk::Result::eErrorOutOfDateKHR
      || result == vk::Result::eSuboptimalKHR
      || framebufferResized
  ) {
    framebufferResized = false;
    Excal::Swapchain::recreateSwapchain(
      window,                   descriptorPool,       commandBuffers,
      swapchain,                swapchainImageFormat, swapchainExtent,
      swapchainImages,          swapchainImageViews,  swapchainFramebuffers,
      colorResources,           depthResources,       uniformBuffers,
      uniformBufferAllocations, renderPass,           graphicsPipeline,
      pipelineLayout,           pipelineCache,        descriptorSets,
      device,                   physicalDevice,       allocator,
      surface,                  msaaSamples,          depthFormat,
      commandPool,              indexCounts,          indexBuffer,
      vertexBuffer,             descriptorSetLayout,  textureSampler,
      textureImageView
    );
    return;
  }

  currentFrame = (currentFrame + 1) % maxFramesInFlight;
}
}
