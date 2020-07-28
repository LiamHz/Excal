#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <vector>

namespace Excal
{
class Context
{
// Type declarations and definitions
public:
  struct DebugContext {
    bool                               enableValidationLayers;
    bool                               validationLayerSupport;
    std::vector<const char*>           validationLayers;
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
  };

  struct SurfaceContext {
    vk::SurfaceKHR surface;
  };

public:
  Context();

  void setDebugContext(const DebugContext&);
  void setSurfaceContext(const SurfaceContext&);

  DebugContext   debug;
  SurfaceContext surface;
};
}
