#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "utils.h"

namespace Excal
{
class Swapchain
{
private:
  Excal::Utils* excalUtils;

public:
  Swapchain(Excal::Utils*);

  struct SwapchainState {
    vk::SwapchainKHR           swapchain;
    vk::Format                 swapchainImageFormat;
    vk::Extent2D               swapchainExtent;
    //std::vector<vk::Image>     swapchainImages;
    //std::vector<vk::ImageView> swapchainImageViews;
  };

  SwapchainState createSwapchain(
    const vk::PhysicalDevice&,
    const vk::Device&,
    const vk::SurfaceKHR&,
    GLFWwindow*
  );

  std::vector<vk::ImageView> createImageViews(
    const vk::Device&,
    const std::vector<vk::Image>&,
    const vk::Format&
  );

  vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>&);
  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR&, GLFWwindow*);
  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>&
  );
};
}
