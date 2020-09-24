#include "surface.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "engine.h"

namespace Excal::Surface
{
GLFWwindow* initWindow(
  Excal::Engine* engine,
  const uint32_t windowWidth,
  const uint32_t windowHeight,
  const char*    windowName
) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  auto window = glfwCreateWindow(
    windowWidth, windowHeight, windowName,
    nullptr,     nullptr
  );

  glfwSetWindowUserPointer(window, engine);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

  return window;
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

void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto engine = static_cast<Excal::Engine*>(glfwGetWindowUserPointer(window));
  engine->framebufferResized = true;
}
}
