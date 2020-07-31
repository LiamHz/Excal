#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>

#include "structs.h"

namespace Excal::Utils
{
SwapChainSupportDetails querySwapChainSupport(
  const vk::PhysicalDevice&,
  const vk::SurfaceKHR&
);

uint32_t findMemoryType(
  const vk::PhysicalDevice&      physicalDevice,
  const uint32_t                 typeFilter,
  const vk::MemoryPropertyFlags& properties
);
}
