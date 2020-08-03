#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace Excal::Surface
{
GLFWwindow* initWindow(
  bool *framebufferResized,
  const uint32_t windowWidth,
  const uint32_t windowHeight,
  const char*    windowName
);

vk::SurfaceKHR createSurface(const vk::Instance&, GLFWwindow*);

void framebufferResizeCallback(GLFWwindow* window, int width, int height);
}
