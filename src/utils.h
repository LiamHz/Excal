#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>

#include "structs.h"
#include "excalInfoStructs.h"

class ExcalUtils
{
private:
  ExcalDebugInfo excalDebugInfo;
  ExcalSurfaceInfo* excalSurfaceInfo;

public:
  ExcalUtils(const ExcalDebugInfo& debugInfo, ExcalSurfaceInfo* surfaceInfo);

  QueueFamilyIndices findQueueFamilies(
    vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface
  );

  std::vector<const char*> getRequiredExtensions();

  SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice);
};
