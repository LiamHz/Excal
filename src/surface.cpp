#include "surface.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Excal::Surface
{
GLFWwindow* initWindow(
  const uint32_t windowWidth,
  const uint32_t windowHeight
) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  return glfwCreateWindow(windowWidth, windowHeight, "Excal", nullptr, nullptr);
  //glfwSetWindowUserPointer(window, this);
  //glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

vk::SurfaceKHR createSurface(
  const vk::Instance& instance,
  GLFWwindow* window
) {
  VkSurfaceKHR vSurface;

  if (glfwCreateWindowSurface(instance, window, nullptr, &vSurface) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create window surface!");
  }

  return vk::SurfaceKHR(vSurface);
}

// TODO Figure out how to pass the type ExcalApplication in
//      and re-enable framebufferResizeCallback
/*
void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto app = static_cast<ExcalApplication*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}
*/
}
