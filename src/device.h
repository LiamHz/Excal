#pragma once

#include <vulkan/vulkan.hpp>

#include "utils.h"
#include "context.h"

namespace Excal
{
class Device
{
private:
  vk::Instance            instance;
  vk::PhysicalDevice      physicalDevice;
  vk::Device              device;
  vk::Queue               graphicsQueue;
  vk::Queue               presentQueue;
  vk::SampleCountFlagBits msaaSamples;

  Excal::Context* context;
  Excal::Utils*   excalUtils;

  int  rateDeviceSuitability(vk::PhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice);

  vk::SampleCountFlagBits getMaxUsableSampleCount();

  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

public:
  Device(Excal::Context*, Excal::Utils*);
  void updateContext(Excal::Context& context);

  void createInstance();
  void pickPhysicalDevice();
  void createLogicalDevice();
};
}
