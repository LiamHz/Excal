#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "excalInfoStructs.h"

class ExcalSurface
{
private:
  GLFWwindow*    window;
  vk::SurfaceKHR surface;

  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

  const uint32_t WIDTH  = 1440;
  const uint32_t HEIGHT = 900;

public:
  ExcalSurface();
  ExcalSurfaceInfo getExcalSurfaceInfo();

  void createSurface(const vk::Instance& instance);
  void initWindow();

  vk::SurfaceKHR getSurface() const { return surface; }
  GLFWwindow*    getWindow()  const { return window;  }
};
