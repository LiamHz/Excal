#include "device.h"

#include "utils.h"
#include "excalInfoStructs.h"

#include <vulkan/vulkan.hpp>
#include <set>
#include <map>
#include <iostream>

ExcalDevice::ExcalDevice(
  const ExcalDebugInfo& debugInfo,
  ExcalSurfaceInfo* surfaceInfo,
  ExcalUtils* excalUtils
) : excalUtils(excalUtils)
{
  excalDebugInfo = debugInfo;
  excalSurfaceInfo = surfaceInfo;
}

void ExcalDevice::createInstance()
{
  if (excalDebugInfo.enableValidationLayers && !excalDebugInfo.validationLayerSupport) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  vk::ApplicationInfo appInfo("Excal Test", 1, "Excal", 1, VK_API_VERSION_1_2);

  // Specify which global extensions to use
  auto extensions = excalUtils->getRequiredExtensions();

  vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(), &appInfo);
  createInfo.enabledExtensionCount   = extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (excalDebugInfo.enableValidationLayers)
  {
    createInfo.enabledLayerCount   = excalDebugInfo.validationLayers.size();
    createInfo.ppEnabledLayerNames = excalDebugInfo.validationLayers.data();

    auto debugCreateInfo = excalDebugInfo.debugMessengerCreateInfo;
    createInfo.pNext     = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
  }
  else
  {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  instance = vk::createInstance(createInfo);
}

void ExcalDevice::pickPhysicalDevice()
{
  auto physicalDevices = instance.enumeratePhysicalDevices();

  if (physicalDevices.size() == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  // Create map of physical devices sorted by rateDeviceSuitability()
  std::multimap<int, vk::PhysicalDevice> candidates;

  for (const auto& physicalDevice : physicalDevices) {
    int score = rateDeviceSuitability(physicalDevice);
    candidates.insert(std::make_pair(score, physicalDevice));
  }

  // Check if best candidate is suitable at all
  if (candidates.rbegin()->first > 0) {
    physicalDevice = candidates.rbegin()->second;
    msaaSamples = getMaxUsableSampleCount();
  } else {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void ExcalDevice::createLogicalDevice()
{
  QueueFamilyIndices indices
    = excalUtils->findQueueFamilies(physicalDevice, excalSurfaceInfo->surface);

  std::set<uint32_t> uniqueQueueFamilies = {
    indices.graphicsFamily.value(),
    indices.presentFamily.value()
  };

  // Select queue families to create
  float queuePriority = 1.0f;
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

  for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamily, 1, &queuePriority);
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // Specify device features application will use
  vk::PhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;
  //deviceFeatures.sampleRateShading = VK_TRUE; // Enable sample shading (interior AA)

  device = physicalDevice.createDevice(
    vk::DeviceCreateInfo(
      {}, queueCreateInfos.size(), queueCreateInfos.data(),
      0, nullptr, // Enabled layers
      deviceExtensions.size(), deviceExtensions.data(),
      &deviceFeatures
    )
  );

  graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
  presentQueue  = device.getQueue(indices.presentFamily.value(), 0);
}

int ExcalDevice::rateDeviceSuitability(vk::PhysicalDevice physicalDevice)
{
  vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
  vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();

  int score = 0;

  // Specify deviceProperties that are preferred
  score += deviceProperties.limits.maxImageDimension2D;
  if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
    score += 1000;
  }

  // Specify deviceFeatures and extension support that application requires
  if (   !deviceFeatures.shaderInt16
      || !deviceFeatures.samplerAnisotropy
      || !checkDeviceExtensionSupport(physicalDevice)
  ) {
    return 0;
  }

  SwapChainSupportDetails swapChainSupport
    = excalUtils->querySwapChainSupport(physicalDevice);
  if (   swapChainSupport.surfaceFormats.empty()
      || swapChainSupport.presentModes.empty()
  ) {
    return 0;
  }

  QueueFamilyIndices indices
    = excalUtils->findQueueFamilies(physicalDevice, excalSurfaceInfo->surface);
  if (!indices.isComplete()) {
    return 0;
  }

  return score;
}

bool ExcalDevice::checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice) {
  auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

  std::set<std::string> requiredExtensions(
    deviceExtensions.begin(), deviceExtensions.end()
  );

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(static_cast<std::string>(extension.extensionName));
  }

  return requiredExtensions.empty();
}

vk::SampleCountFlagBits ExcalDevice::getMaxUsableSampleCount() {
  auto deviceProperties = physicalDevice.getProperties();

  vk::SampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts
                               & deviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
  if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
  if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
  if (counts & vk::SampleCountFlagBits::e8)  { return vk::SampleCountFlagBits::e8;  }
  if (counts & vk::SampleCountFlagBits::e4)  { return vk::SampleCountFlagBits::e4;  }
  if (counts & vk::SampleCountFlagBits::e2)  { return vk::SampleCountFlagBits::e2;  }

  return vk::SampleCountFlagBits::e1;
}

