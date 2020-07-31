#include "device.h"

#include "utils.h"

#include <vulkan/vulkan.hpp>
#include <set>
#include <map>
#include <iostream>

namespace Excal::Device
{
vk::Instance createInstance(
  const bool                                validationLayersEnabled,
  const bool                                validationLayersSupported,
  const std::vector<const char*>&           validationLayers,
  const VkDebugUtilsMessengerCreateInfoEXT& debugMessengerCreateInfo
) {
  if (validationLayersEnabled && !validationLayersSupported) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  vk::ApplicationInfo appInfo("Excal Test", 1, "Excal", 1, VK_API_VERSION_1_2);

  // Specify which global extensions to use
  auto extensions = getRequiredExtensions(validationLayersEnabled);

  vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(), &appInfo);
  createInfo.enabledExtensionCount   = extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (validationLayersEnabled)
  {
    createInfo.enabledLayerCount   = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();

    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerCreateInfo;
  }
  else
  {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  return vk::createInstance(createInfo);
}

vk::PhysicalDevice pickPhysicalDevice(
const vk::Instance&   instance,
const vk::SurfaceKHR& surface
) {
  auto physicalDevices = instance.enumeratePhysicalDevices();

  if (physicalDevices.size() == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  // Create map of physical devices sorted by rateDeviceSuitability()
  std::multimap<int, vk::PhysicalDevice> candidates;

  for (const auto& physicalDevice : physicalDevices) {
    int score = rateDeviceSuitability(physicalDevice, surface);
    candidates.insert(std::make_pair(score, physicalDevice));
  }

  // Check if best candidate is suitable at all
  if (candidates.rbegin()->first == 0) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  return candidates.rbegin()->second;
}

QueueFamilyIndices findQueueFamilies(
  const vk::PhysicalDevice& physicalDevice,
  const vk::SurfaceKHR&     surface
) {
  QueueFamilyIndices indices;
  auto queueFamilies = physicalDevice.getQueueFamilyProperties();

  // Prefer a queue family with both graphics capabilities and surface support
  // Otherwise track the indices of the two different queue families that, together,
  // support both of these things
  int i = 0;
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
    {
      indices.graphicsFamily = i;
    }

    vk::Bool32 presentSupport = false;
    presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface);

    if (presentSupport)
    {
      indices.presentFamily = i;
    }

    if (indices.isComplete())
    {
      break;
    }

    i++;
  }

  return indices;
}

vk::Device createLogicalDevice(
  const vk::PhysicalDevice& physicalDevice,
  const QueueFamilyIndices& indices
) {
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

  auto deviceExtensions = getDeviceExtensions();

  return physicalDevice.createDevice(
    vk::DeviceCreateInfo(
      {}, queueCreateInfos.size(), queueCreateInfos.data(),
      0, nullptr, // Enabled layers
      deviceExtensions.size(), deviceExtensions.data(),
      &deviceFeatures
    )
  );
}

int rateDeviceSuitability(
  const vk::PhysicalDevice& physicalDevice,
  const vk::SurfaceKHR&     surface
) {
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
    = Excal::Utils::querySwapChainSupport(physicalDevice, surface);
  if (   swapChainSupport.surfaceFormats.empty()
      || swapChainSupport.presentModes.empty()
  ) {
    return 0;
  }

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
  if (!indices.isComplete()) {
    return 0;
  }

  return score;
}

std::vector<const char*> getRequiredExtensions(
  const bool validationLayersEnabled
) {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(
    glfwExtensions, glfwExtensions + glfwExtensionCount
  );

  if (validationLayersEnabled)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

std::vector<const char*> getDeviceExtensions() {
  return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

bool checkDeviceExtensionSupport(
  const vk::PhysicalDevice& physicalDevice
) {
  auto deviceExtensions    = getDeviceExtensions();
  auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

  std::set<std::string> requiredExtensions(
    deviceExtensions.begin(), deviceExtensions.end()
  );

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(static_cast<std::string>(extension.extensionName));
  }

  return requiredExtensions.empty();
}

vk::SampleCountFlagBits getMaxUsableSampleCount(
  const vk::PhysicalDevice& physicalDevice
) {
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
}
