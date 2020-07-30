#include "utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <optional>
#include <iostream>

#include "structs.h"

namespace Excal::Utils
{
std::vector<const char*> getRequiredExtensions(
  const bool validationLayersEnabled
) {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(
    glfwExtensions, glfwExtensions + glfwExtensionCount
  );

  if (validationLayersEnabled)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

SwapChainSupportDetails querySwapChainSupport(
  const vk::PhysicalDevice& physicalDevice,
  const vk::SurfaceKHR&     surface
) {
  SwapChainSupportDetails details;
  details.presentModes        = physicalDevice.getSurfacePresentModesKHR(surface);
  details.surfaceFormats      = physicalDevice.getSurfaceFormatsKHR(surface);
  details.surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

  return details;
}

vk::ImageView createImageView(
  const vk::Device&           device,
  const vk::Image&            image,
  const vk::Format&           format,
  const vk::ImageAspectFlags& aspectFlags
) {
  auto imageView = device.createImageView(
    vk::ImageViewCreateInfo(
      {}, image, vk::ImageViewType::e2D, format,
      vk::ComponentMapping(vk::ComponentSwizzle::eIdentity),
      vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1)
    )
  );

  return imageView;
}
}
