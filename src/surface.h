#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace Excal
{
class Surface
{
public:
  Surface();

  GLFWwindow*    initWindow(const uint32_t windowWidth, const uint32_t windowHeight);
  vk::SurfaceKHR createSurface(const vk::Instance&, GLFWwindow*);

  //static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};
}
