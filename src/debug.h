#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

#include "context.h"

namespace Excal
{
class Debug
{
public:
  Debug();
  Excal::Context::DebugContext getContext();

  bool checkValidationLayerSupport();

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
  );

  VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();

  //#define NDEBUG
  #ifdef NDEBUG
    const bool enableValidationLayers = false;
  #else
    const bool enableValidationLayers = true;
  #endif

  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };
};
}
