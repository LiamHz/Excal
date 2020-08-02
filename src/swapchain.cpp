#include "swapchain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <tuple>

#include "structs.h"
#include "image.h"
#include "device.h"
#include "buffer.h"
#include "descriptor.h"
#include "pipeline.h"

namespace Excal::Swapchain
{
Excal::Swapchain::SwapchainState createSwapchain(
  const vk::PhysicalDevice& physicalDevice,
  const vk::Device&         device,
  const vk::SurfaceKHR&     surface,
  GLFWwindow*               window,
  const QueueFamilyIndices& indices
) {
  SwapchainSupportDetails swapchainSupport
    = Excal::Device::querySwapchainSupport(physicalDevice, surface);

  auto surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.surfaceFormats);
  auto presentMode   = chooseSwapPresentMode(swapchainSupport.presentModes);
  auto extent        = chooseSwapExtent(swapchainSupport.surfaceCapabilities, window);

  uint32_t imageCount = swapchainSupport.surfaceCapabilities.minImageCount + 1;
  if (  swapchainSupport.surfaceCapabilities.maxImageCount > 0
     && imageCount > swapchainSupport.surfaceCapabilities.maxImageCount
  ) {
    imageCount = swapchainSupport.surfaceCapabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo(
    {}, surface, imageCount,
    surfaceFormat.format, surfaceFormat.colorSpace, extent,
    1, vk::ImageUsageFlagBits::eColorAttachment
  );
  createInfo.preTransform = swapchainSupport.surfaceCapabilities.currentTransform;

  uint32_t queueFamilyIndices[] = {
    indices.graphicsFamily.value(),
    indices.presentFamily.value()
  };

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;
  }

  return SwapchainState {
    device.createSwapchainKHR(createInfo),
    surfaceFormat.format,
    extent
  };
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
  const std::vector<vk::SurfaceFormatKHR>& availableFormats
) {
  // Prefer SRGB color format if available
  for (const auto& availableFormat : availableFormats) {
    if (  availableFormat.format     == vk::Format::eB8G8R8A8Srgb
       && availableFormat.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear
    ) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(
  const std::vector<vk::PresentModeKHR>& availablePresentModes
) {
  // Prefer triple buffering if available
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    }
  }

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(
  const vk::SurfaceCapabilitiesKHR& capabilities,
  GLFWwindow*                       window
) {
  // Note: Swap extent is the resolution of the swap chain images.
  //       Almost always equal to the resolution of the window being drawn to.

  // Window managers that allow extent to differ from resolution
  // of the window being drawn to set currentExtent to UINT32_MAX
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };

    // Clamp value of WIDTH and HEIGHT between the min and max
    // extents supported by the implementation
    auto maxWidth = std::min(capabilities.maxImageExtent.width, actualExtent.width);
    actualExtent.width = std::max(capabilities.minImageExtent.width, maxWidth);

    auto maxHeight = std::min(capabilities.maxImageExtent.height, actualExtent.height);
    actualExtent.height = std::max(capabilities.minImageExtent.height, maxHeight);

    return actualExtent;
  }
}

void cleanupSwapchain(
  const vk::Device&               device,
  const vk::CommandPool&          commandPool,
  vk::DescriptorPool&             descriptorPool,
  std::vector<vk::CommandBuffer>& commandBuffers,
  vk::SwapchainKHR&               swapchain,
  std::vector<vk::ImageView>&     swapchainImageViews,
  std::vector<VkFramebuffer>&     swapchainFramebuffers,
  Excal::Image::ImageResources&   colorResources,
  Excal::Image::ImageResources&   depthResources,
  std::vector<vk::Buffer>&        uniformBuffers,
  std::vector<vk::DeviceMemory>&  uniformBuffersMemory,
  vk::RenderPass&                 renderPass,
  vk::Pipeline&                   graphicsPipeline,
  vk::PipelineCache&              pipelineCache,
  vk::PipelineLayout&             pipelineLayout
) {
  device.destroyImageView(colorResources.imageView);
  device.destroyImage(colorResources.image);
  device.freeMemory(colorResources.imageMemory);

  device.destroyImageView(depthResources.imageView);
  device.destroyImage(depthResources.image);
  device.freeMemory(depthResources.imageMemory);

  for (auto framebuffer : swapchainFramebuffers) {
    device.destroyFramebuffer(framebuffer);
  }

  device.freeCommandBuffers(commandPool, commandBuffers);

  device.destroyDescriptorPool(descriptorPool);

  device.destroyPipeline(graphicsPipeline);
  device.destroyPipelineCache(pipelineCache);
  device.destroyPipelineLayout(pipelineLayout);
  device.destroyRenderPass(renderPass);

  for (size_t i=0; i < swapchainImageViews.size(); i++) {
    device.destroyImageView(swapchainImageViews[i]);
    device.destroyBuffer(uniformBuffers[i]);
    device.freeMemory(uniformBuffersMemory[i]);
  }

  device.destroySwapchainKHR(swapchain);
}

void recreateSwapchain(
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
  std::vector<vk::DeviceMemory>&        uniformBuffersMemory,
  vk::RenderPass&                       renderPass,
  vk::Pipeline&                         graphicsPipeline,
  vk::PipelineLayout&                   pipelineLayout,
  vk::PipelineCache&                    pipelineCache,
  std::vector<vk::DescriptorSet>&       descriptorSets,
  const vk::Device&                     device,
  const vk::PhysicalDevice&             physicalDevice,
  const vk::SurfaceKHR&                 surface,
  const vk::SampleCountFlagBits&        msaaSamples,
  const vk::Format&                     depthFormat,
  const int                             nIndices,
  const vk::CommandPool&                commandPool,
  const vk::Buffer&                     vertexBuffer,
  const vk::Buffer&                     indexBuffer,
  const vk::DescriptorSetLayout&        descriptorSetLayout,
  const vk::ImageView&                  textureImageView,
  const vk::Sampler&                    textureSampler
) {
  // Handle widow minimization
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);

  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  device.waitIdle();

  cleanupSwapchain(
    device,                commandPool,          descriptorPool,
    commandBuffers,        swapchain,            swapchainImageViews,
    swapchainFramebuffers, colorResources,       depthResources,
    uniformBuffers,        uniformBuffersMemory, renderPass,
    graphicsPipeline,      pipelineCache,        pipelineLayout
  );

  auto queueFamilyIndices = Excal::Device::findQueueFamilies(physicalDevice, surface);

  auto swapchainState = Excal::Swapchain::createSwapchain(
    physicalDevice,
    device,
    surface,
    window,
    queueFamilyIndices
  );

  swapchain            = swapchainState.swapchain;
  swapchainImageFormat = swapchainState.swapchainImageFormat;
  swapchainExtent      = swapchainState.swapchainExtent;
  swapchainImages      = device.getSwapchainImagesKHR(swapchain);

  swapchainImageViews = Excal::Image::createImageViews(
    device, swapchainImages, swapchainImageFormat
  );

  // Resource creation
  colorResources = Excal::Image::createColorResources(
    physicalDevice,
    device,
    swapchainImageFormat,
    swapchainExtent,
    msaaSamples
  );

  renderPass = Excal::Pipeline::createRenderPass(
    device,
    depthFormat,
    swapchainImageFormat,
    msaaSamples
  );

  // TODO Fill in the create info
  pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

  pipelineLayout = device.createPipelineLayout(
    vk::PipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 0, nullptr)
  );

  graphicsPipeline = Excal::Pipeline::createGraphicsPipeline(
    device,
    pipelineLayout,
    pipelineCache,
    renderPass,
    swapchainExtent,
    msaaSamples
  );

  depthResources = Excal::Image::createDepthResources(
    physicalDevice,
    device,
    depthFormat,
    swapchainImageFormat,
    swapchainExtent,
    msaaSamples
  );

  swapchainFramebuffers = Excal::Buffer::createFramebuffers(
    device,
    swapchainImageViews,
    colorResources.imageView,
    depthResources.imageView,
    renderPass,
    swapchainExtent
  );

  uniformBuffers = Excal::Buffer::createUniformBuffers(
    uniformBuffersMemory,
    physicalDevice,
    device,
    swapchainImageViews.size()
  );

  descriptorPool = Excal::Descriptor::createDescriptorPool(device, swapchainImages.size());

  descriptorSets = Excal::Descriptor::createDescriptorSets(
    device,
    swapchainImages.size(),
    descriptorPool,
    descriptorSetLayout,
    uniformBuffers,
    textureImageView,
    textureSampler
  );
  
  commandBuffers = Excal::Buffer::createCommandBuffers(
    device,          commandPool, swapchainFramebuffers,
    swapchainExtent, nIndices,    graphicsPipeline,
    vertexBuffer,    indexBuffer, renderPass,
    descriptorSets,  pipelineLayout
  );
}
}
