#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>

#include "structs.h"
#include "context.h"

namespace Excal
{
class Utils
{
private:
  Excal::Context* context;

public:
  Utils(Excal::Context*);

  QueueFamilyIndices findQueueFamilies(
    vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface
  );

  std::vector<const char*> getRequiredExtensions();

  SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice);

  vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
};
}
