#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "context.h"
#include "utils.h"

namespace Excal
{
class Swapchain
{
private:
  Excal::Context*    context;
  Excal::Utils*      excalUtils;

  vk::SwapchainKHR               swapchain;
  vk::Format                     swapchainImageFormat;
  vk::Extent2D                   swapchainExtent;
  std::vector<vk::Image>         swapchainImages;
  std::vector<vk::ImageView>     swapchainImageViews;

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    std::vector<vk::SurfaceFormatKHR> availableFormats
  );

  vk::PresentModeKHR chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes
  );

  vk::Extent2D chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities
  );

public:
  Swapchain(Excal::Context*, Excal::Utils*);
  void updateContext(Excal::Context& context);

  void createSwapChain();
  void createImageViews();
};
}
