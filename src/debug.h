#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <iostream>

namespace Excal::Debug
{
bool checkValidationLayerSupport();
std::vector<const char*> getValidationLayers();
VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData
);

VkResult CreateDebugUtilsMessengerEXT(
  const VkInstance&                         instance,
  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks*              pAllocator,
  VkDebugUtilsMessengerEXT*                 pDebugMessenger
);

void DestroyDebugUtilsMessengerEXT(
  const VkInstance&               instance,
  const VkDebugUtilsMessengerEXT& debugMessenger,
  const VkAllocationCallbacks*    pAllocator
);

VkDebugUtilsMessengerEXT setupDebugMessenger(
  const vk::Instance&                       instance,
  const bool                                validationLayersEnabled,
  const VkDebugUtilsMessengerCreateInfoEXT& debugMessengerCreateInfo
);
}
