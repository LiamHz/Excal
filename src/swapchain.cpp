#include "swapchain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
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
}
