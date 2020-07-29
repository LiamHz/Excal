#include "swapchain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "context.h"
#include "utils.h"

namespace Excal
{
Swapchain::Swapchain(Excal::Context* context, Excal::Utils* excalUtils)
  : context(context), excalUtils(excalUtils) {}

void Swapchain::updateContext(Excal::Context& context) {
  context.swapchain = {
    swapchain,
    swapchainImageFormat,
    swapchainExtent,
    swapchainImages,
    swapchainImageViews
  };
}

void Swapchain::createSwapChain() {
  SwapChainSupportDetails swapchainSupport
    = excalUtils->querySwapChainSupport(context->device.physicalDevice);

  auto surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.surfaceFormats);
  auto presentMode   = chooseSwapPresentMode(swapchainSupport.presentModes);
  auto extent        = chooseSwapExtent(swapchainSupport.surfaceCapabilities);

  uint32_t imageCount = swapchainSupport.surfaceCapabilities.minImageCount + 1;
  if (  swapchainSupport.surfaceCapabilities.maxImageCount > 0
     && imageCount > swapchainSupport.surfaceCapabilities.maxImageCount
  ) {
    imageCount = swapchainSupport.surfaceCapabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo(
    {}, context->surface.surface, imageCount,
    surfaceFormat.format, surfaceFormat.colorSpace, extent,
    1, vk::ImageUsageFlagBits::eColorAttachment
  );
  createInfo.preTransform = swapchainSupport.surfaceCapabilities.currentTransform;

  QueueFamilyIndices indices = excalUtils->findQueueFamilies(
    context->device.physicalDevice,
    context->surface.surface
  );

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

  swapchain            = context->device.device.createSwapchainKHR(createInfo);
  swapchainImages      = context->device.device.getSwapchainImagesKHR(swapchain);
  swapchainImageFormat = surfaceFormat.format;
  swapchainExtent      = extent;
}

void Swapchain::createImageViews() {
  swapchainImageViews.resize(swapchainImages.size());

  for (size_t i=0; i < swapchainImages.size(); i++) {
    swapchainImageViews[i] = excalUtils->createImageView(
      swapchainImages[i], swapchainImageFormat, vk::ImageAspectFlagBits::eColor
    );
  }
}

vk::SurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(
  std::vector<vk::SurfaceFormatKHR> availableFormats
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

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(
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

vk::Extent2D Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
  // Note: Swap extent is the resolution of the swap chain images.
  //       Almost always equal to the resolution of the window being drawn to.

  // Window managers that allow extent to differ from resolution
  // of the window being drawn to set currentExtent to UINT32_MAX
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(context->surface.window, &width, &height);

    vk::Extent2D actualExtent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };

    // Clamp value of WIDTH and HEIGHT between the min and max
    // extents supported by the implementation
    actualExtent.width = std::max(capabilities.minImageExtent.width,
                                  std::min(capabilities.maxImageExtent.width,
                                           actualExtent.width));

    actualExtent.height = std::max(capabilities.minImageExtent.height,
                                   std::min(capabilities.maxImageExtent.height,
                                            actualExtent.height));

    return actualExtent;
  }
}
}
