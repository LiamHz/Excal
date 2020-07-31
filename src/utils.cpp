#include "utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <optional>
#include <iostream>

#include "structs.h"

namespace Excal::Utils
{

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

uint32_t findMemoryType(
  const vk::PhysicalDevice&      physicalDevice,
  const uint32_t                 typeFilter,
  const vk::MemoryPropertyFlags& properties
) {
  auto memProperties = physicalDevice.getMemoryProperties();

  for (uint32_t i=0; i < memProperties.memoryTypeCount; i++) {
    if (   typeFilter & (1 << i)
        && (memProperties.memoryTypes[i].propertyFlags & properties) == properties
    ) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}
}
