#pragma once

#include <vulkan/vulkan.hpp>

#include "utils.h"

namespace Excal
{
class Device
{
private:
  Excal::Utils*   excalUtils;

public:
  Device(Excal::Utils*);

  vk::Instance createInstance(
    const bool validationLayersEnabled,
    const bool validationLayersSupported,
    const std::vector<const char*>&,
    const VkDebugUtilsMessengerCreateInfoEXT&
  );

  vk::PhysicalDevice pickPhysicalDevice(const vk::Instance&, const vk::SurfaceKHR&);
  vk::Device createLogicalDevice(const vk::PhysicalDevice&, const vk::SurfaceKHR&);

  vk::Queue getGraphicsQueue(
    const vk::PhysicalDevice&,
    const vk::Device&,
    const vk::SurfaceKHR&
  );

  vk::Queue getPresentQueue(
    const vk::PhysicalDevice&,
    const vk::Device&,
    const vk::SurfaceKHR&
  );

  std::vector<const char*> getDeviceExtensions();
  bool checkDeviceExtensionSupport(const vk::PhysicalDevice& physicalDevice);
  int  rateDeviceSuitability(const vk::PhysicalDevice&, const vk::SurfaceKHR&);
  vk::SampleCountFlagBits getMaxUsableSampleCount(const vk::PhysicalDevice&);
};
}
