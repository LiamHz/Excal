#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>

#include "structs.h"

namespace Excal
{
class Utils
{
public:
  Utils();

  QueueFamilyIndices findQueueFamilies(
    const vk::PhysicalDevice&,
    const vk::SurfaceKHR&
  );

  std::vector<const char*> getRequiredExtensions(const bool validationLayersEnabled);

  SwapChainSupportDetails querySwapChainSupport(
    const vk::PhysicalDevice&,
    const vk::SurfaceKHR&
  );

  vk::ImageView createImageView(
    const vk::Device&,
    const vk::Image&,
    const vk::Format&,
    const vk::ImageAspectFlags&
  );
};
}
