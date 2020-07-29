#include "utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <optional>
#include <iostream>

#include "structs.h"
#include "context.h"

namespace Excal
{
Utils::Utils(Excal::Context* context) : context(context) {}

QueueFamilyIndices Utils::findQueueFamilies(
  vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface
) {
  QueueFamilyIndices indices;
  auto queueFamilies = physicalDevice.getQueueFamilyProperties();

  // Prefer a queue family with both graphics capabilities and surface support
  // Otherwise track the indices of the two different queue families that, together,
  // support both of these things
  int i = 0;
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
    {
      indices.graphicsFamily = i;
    }

    vk::Bool32 presentSupport = false;
    presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface);

    if (presentSupport)
    {
      indices.presentFamily = i;
    }

    if (indices.isComplete())
    {
      break;
    }

    i++;
  }

  return indices;
}

std::vector<const char*> Utils::getRequiredExtensions()
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(
    glfwExtensions, glfwExtensions + glfwExtensionCount
  );

  // TODO glfwGetRequiredInstanceExtensions isn't returning any values
  //      The following two lines are a macOS specific workaround
  extensions.push_back("VK_KHR_surface");
  extensions.push_back("VK_EXT_metal_surface");

  if (context->debug.enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

SwapChainSupportDetails Utils::querySwapChainSupport(vk::PhysicalDevice physicalDevice)
{
  SwapChainSupportDetails details;
  details.surfaceCapabilities
    = physicalDevice.getSurfaceCapabilitiesKHR(context->surface.surface);

  details.surfaceFormats
    = physicalDevice.getSurfaceFormatsKHR(context->surface.surface);

  details.presentModes
    = physicalDevice.getSurfacePresentModesKHR(context->surface.surface);

  return details;
}

vk::ImageView Utils::createImageView(
  vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags
) {
  auto imageView = context->device.device.createImageView(
    vk::ImageViewCreateInfo(
      {}, image, vk::ImageViewType::e2D, format,
      vk::ComponentMapping(vk::ComponentSwizzle::eIdentity),
      vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1)
    )
  );

  return imageView;
}
}
