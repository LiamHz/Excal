#pragma once

#include <vulkan/vulkan.hpp>

#include "structs.h"

namespace Excal::Device
{
vk::Instance createInstance(
  const vk::ApplicationInfo&,
  const bool validationLayersEnabled,
  const bool validationLayersSupported,
  const std::vector<const char*>&,
  const VkDebugUtilsMessengerCreateInfoEXT&
);

vk::PhysicalDevice pickPhysicalDevice(const vk::Instance&, const vk::SurfaceKHR&);
QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice&, const vk::SurfaceKHR&);
vk::Device createLogicalDevice(const vk::PhysicalDevice&, const QueueFamilyIndices&);

std::vector<const char*> getRequiredExtensions(const bool validationLayersEnabled);
std::vector<const char*> getDeviceExtensions();
bool checkDeviceExtensionSupport(const vk::PhysicalDevice& physicalDevice);

int rateDeviceSuitability(const vk::PhysicalDevice&, const vk::SurfaceKHR&);
vk::SampleCountFlagBits getMaxUsableSampleCount(const vk::PhysicalDevice&);

SwapchainSupportDetails querySwapchainSupport(
  const vk::PhysicalDevice&,
  const vk::SurfaceKHR&
);

uint32_t findMemoryType(
  const vk::PhysicalDevice&      physicalDevice,
  const uint32_t                 typeFilter,
  const vk::MemoryPropertyFlags& properties
);
}
