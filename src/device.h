#pragma once

#include <vulkan/vulkan.hpp>
#include <tuple>

#include "utils.h"
#include "excalInfoStructs.h"

class ExcalDevice
{
private:
  vk::Instance            instance;
  vk::Device              device;
  vk::PhysicalDevice      physicalDevice;
  vk::Queue               graphicsQueue;
  vk::Queue               presentQueue;
  vk::SampleCountFlagBits msaaSamples;

  ExcalDebugInfo    excalDebugInfo;
  ExcalSurfaceInfo* excalSurfaceInfo;

  ExcalUtils* excalUtils;

  int  rateDeviceSuitability(vk::PhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice);

  vk::SampleCountFlagBits getMaxUsableSampleCount();

  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

public:
  ExcalDevice(
    const ExcalDebugInfo& debugInfo,
    ExcalSurfaceInfo* surfaceInfo,
    ExcalUtils* excalUtils
  );

  vk::Instance createInstance();
  std::tuple<vk::PhysicalDevice, vk::SampleCountFlagBits> pickPhysicalDevice();
  std::tuple<vk::Device, vk::Queue, vk::Queue>            createLogicalDevice();
};
