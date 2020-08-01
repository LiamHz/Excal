#include "frame.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "swapchain.h"
#include "buffer.h"
#include "image.h"

namespace Excal::Frame
{
void drawFrame(
  // Required for call to Excal::Swapchain::recreateSwpachain
  // but otherwise not for drawFrame()
  GLFWwindow*                           window,
  vk::DescriptorPool&                   descriptorPool,
  std::vector<vk::CommandBuffer>&       commandBuffers,
  vk::SwapchainKHR&                     swapchain,
  vk::Format&                           swapchainImageFormat,
  vk::Extent2D                          swapchainExtent,
  std::vector<vk::Image>&               swapchainImages,
  std::vector<vk::ImageView>&           swapchainImageViews,
  std::vector<VkFramebuffer>&           swapchainFramebuffers,
  Excal::Image::ImageResources&         colorResources,
  Excal::Image::ImageResources&         depthResources,
  std::vector<vk::Buffer>&              uniformBuffers,
  const vk::PhysicalDevice&             physicalDevice,
  const vk::SurfaceKHR&                 surface,
  const vk::SampleCountFlagBits&        msaaSamples,
  const vk::Format&                     depthFormat,
  const vk::RenderPass&                 renderPass,
  const int                             nIndices,
  const vk::CommandPool&                commandPool,
  const vk::Pipeline&                   graphicsPipeline,
  const vk::Buffer&                     vertexBuffer,
  const vk::Buffer&                     indexBuffer,
  const vk::PipelineLayout              pipelineLayout,
  const std::vector<vk::DescriptorSet>& descriptorSets,

  // Required for regular drawFrame() functionality
  size_t                            currentFrame,
  std::vector<vk::DeviceMemory>&    uniformBuffersMemory,
  std::vector<vk::Fence>&           imagesInFlight,
  const vk::Device&                 device,
  const vk::Queue&                  graphicsQueue,
  const vk::Queue&                  presentQueue,
  const std::vector<vk::Fence>&     inFlightFences,
  const std::vector<vk::Semaphore>& imageAvailableSemaphores,
  const std::vector<vk::Semaphore>& renderFinishedSemaphores,
  const bool                        framebufferResized,
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
    Excal::Swapchain::recreateSwapChain(
      window,               descriptorPool,       commandBuffers,
      swapchain,            swapchainImageFormat, swapchainExtent,
      swapchainImages,      swapchainImageViews,  swapchainFramebuffers,
      colorResources,       depthResources,       uniformBuffers,
      uniformBuffersMemory, device,               physicalDevice,
      surface,              msaaSamples,          depthFormat,
      renderPass,           nIndices,             commandPool,
      graphicsPipeline,     vertexBuffer,         indexBuffer,
      pipelineLayout,       descriptorSets
    );
    return;
  }

  Excal::Buffer::updateUniformBuffer(
    uniformBuffersMemory,
    device,
    swapchainExtent,
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
    Excal::Swapchain::recreateSwapChain(
      window,               descriptorPool,       commandBuffers,
      swapchain,            swapchainImageFormat, swapchainExtent,
      swapchainImages,      swapchainImageViews,  swapchainFramebuffers,
      colorResources,       depthResources,       uniformBuffers,
      uniformBuffersMemory, device,               physicalDevice,
      surface,              msaaSamples,          depthFormat,
      renderPass,           nIndices,             commandPool,
      graphicsPipeline,     vertexBuffer,         indexBuffer,
      pipelineLayout,       descriptorSets
    );
    return;
  }

  currentFrame = (currentFrame + 1) % maxFramesInFlight;
}
}
