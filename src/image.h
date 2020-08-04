#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Excal::Image
{
struct ImageResources  {
  vk::Image     image;
  VmaAllocation imageAllocation;
  vk::ImageView imageView;
};

vk::ImageView createImageView(
  const vk::Device&,
  const vk::Image&,
  const vk::Format&,
  const vk::ImageAspectFlags&
);

std::vector<vk::ImageView> createImageViews(
  const vk::Device&             device,
  const std::vector<vk::Image>& images,
  const vk::Format&             imageFormat
);

void transitionImageLayout(
  const vk::Device&      device,
  const vk::CommandPool& commandPool,
  const vk::Queue&       graphicsQueue,
  const vk::Image&       image,
  const vk::Format&      format,
  const vk::ImageLayout& oldLayout,
  const vk::ImageLayout& newLayout
);

void copyBufferToImage(
  const vk::Device&      device,
  const vk::CommandPool& commandPool,
  const vk::Queue&       graphicsQueue,
  const vk::Buffer&      buffer,
  const vk::Image&       image,
  const uint32_t         width,
  const uint32_t         height
);

vk::Image createImage(
  VmaAllocator&                     allocator,
  VmaAllocation&                    imageAllocation,
  VmaAllocationCreateInfo&          allocInfo,
  const vk::PhysicalDevice&         physicalDevice,
  const vk::Device&                 device,
  const uint32_t                    width,
  const uint32_t                    height,
  const vk::SampleCountFlagBits&    numSamples,
  const vk::Format&                 format,
  const vk::ImageTiling&            tiling,
  const vk::ImageUsageFlags&        usage,
  const vk::MemoryPropertyFlagBits& properties
);

ImageResources createColorResources(
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  VmaAllocator&                  allocator,
  const vk::Format&              swapchainImageFormat,
  const vk::Extent2D&            swapchainExtent,
  const vk::SampleCountFlagBits& msaaSamples
);

ImageResources createDepthResources(
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  VmaAllocator&                  allocator,
  const vk::Format&              depthFormat,
  const vk::Format&              swapchainImageFormat,
  const vk::Extent2D&            swapchainExtent,
  const vk::SampleCountFlagBits& msaaSamples
);

ImageResources createTextureResources(
  const vk::PhysicalDevice& physicalDevice,
  const vk::Device&         device,
  VmaAllocator&             allocator,
  const vk::CommandPool&    commandPool,
  const vk::Queue&          graphicsQueue,
  const std::string&        texturePath
);

vk::Sampler createTextureImageSampler(
  const vk::Device& device,
  const vk::Image&  textureImage
);

vk::Format findSupportedFormat(
  const vk::PhysicalDevice&      physicalDevice,
  const std::vector<vk::Format>& candidates,
  const vk::ImageTiling&         tiling,
  const vk::FormatFeatureFlags&  features
);

vk::Format findDepthFormat(
  const vk::PhysicalDevice& physicalDevice
);
}
