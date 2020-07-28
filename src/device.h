#pragma once

#include <vulkan/vulkan.hpp>

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

  ExcalDebugInfo   excalDebugInfo;
  ExcalSurfaceInfo* excalSurfaceInfo;

  int  rateDeviceSuitability(vk::PhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice);

  vk::SampleCountFlagBits getMaxUsableSampleCount();

  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  ExcalUtils* excalUtils;

public:
  ExcalDevice(
    const ExcalDebugInfo& debugInfo,
    ExcalSurfaceInfo* surfaceInfo,
    ExcalUtils* excalUtils
  );

  void createInstance();
  void pickPhysicalDevice();
  void createLogicalDevice();

  vk::Instance            getInstance()       const { return instance;       }
  vk::PhysicalDevice      getPhysicalDevice() const { return physicalDevice; }
  vk::Device              getDevice()         const { return device;         }
  vk::Queue               getGraphicsQueue()  const { return graphicsQueue;  }
  vk::Queue               getPresentQueue()   const { return presentQueue;   }
  vk::SampleCountFlagBits getMsaaSamples()    const { return msaaSamples;    }
};
