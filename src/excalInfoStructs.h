#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <vector>

struct ExcalDebugInfo {
  bool                               enableValidationLayers;
  bool                               validationLayerSupport;
  std::vector<const char*>           validationLayers;
  VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
};

struct ExcalSurfaceInfo {
  vk::SurfaceKHR surface;
};
