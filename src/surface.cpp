#include "surface.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "excalInfoStructs.h"

ExcalSurface::ExcalSurface()
{
  return;
}

ExcalSurfaceInfo ExcalSurface::getExcalSurfaceInfo()
{
  return ExcalSurfaceInfo {
    surface
  };
}

GLFWwindow* ExcalSurface::initWindow()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "Excal", nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  //glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  return window;
}

vk::SurfaceKHR ExcalSurface::createSurface(const vk::Instance& instance)
{
  auto vSurface = VkSurfaceKHR(surface);

  if (glfwCreateWindowSurface(instance, window, nullptr, &vSurface) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create window surface!");
  }

  surface = vk::SurfaceKHR(vSurface);
  return surface;
}

// TODO Figure out how to pass the type ExcalApplication in
//      and re-enable framebufferResizeCallback
/*
void ExcalSurface::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto app = static_cast<ExcalApplication*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}
*/
