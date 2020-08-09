#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "structs.h"
#include "image.h"

namespace Excal::Swapchain
{
struct SwapchainState {
  vk::SwapchainKHR           swapchain;
  vk::Format                 swapchainImageFormat;
  vk::Extent2D               swapchainExtent;
};

SwapchainState createSwapchain(
  const vk::PhysicalDevice&,
  const vk::Device&,
  const vk::SurfaceKHR&,
  GLFWwindow*,
  const QueueFamilyIndices&
);

vk::PresentModeKHR   chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>&);
vk::Extent2D         chooseSwapExtent(const vk::SurfaceCapabilitiesKHR&, GLFWwindow*);
vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>&);
}
