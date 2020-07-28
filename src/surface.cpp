#include "surface.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "context.h"

namespace Excal
{
Surface::Surface()
{
  return;
}

void Surface::updateContext(Excal::Context& context) {
  context.surface = {
    window,
    surface
  };
}

void Surface::initWindow()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "Excal", nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  //glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Surface::createSurface(const vk::Instance& instance)
{
  auto vSurface = VkSurfaceKHR(surface);

  if (glfwCreateWindowSurface(instance, window, nullptr, &vSurface) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create window surface!");
  }

  surface = vk::SurfaceKHR(vSurface);
}

// TODO Figure out how to pass the type ExcalApplication in
//      and re-enable framebufferResizeCallback
/*
void Surface::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto app = static_cast<ExcalApplication*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}
*/
}
