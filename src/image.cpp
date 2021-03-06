#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vulkan.hpp>
#include <cstring>

#include "buffer.h"
#include "device.h"

namespace Excal::Image
{
vk::ImageView createImageView(
  const vk::Device&           device,
  const vk::Image&            image,
  const vk::Format&           format,
  const vk::ImageAspectFlags& aspectFlags
) {
  auto imageView = device.createImageView(
    vk::ImageViewCreateInfo(
      {}, image, vk::ImageViewType::e2D, format,
      vk::ComponentMapping(vk::ComponentSwizzle::eIdentity),
      vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1)
    )
  );

  return imageView;
}

// TODO Delete and replace calls with single line Vulkan API calls
std::vector<vk::ImageView> createImageViews(
  const vk::Device&             device,
  const std::vector<vk::Image>& images,
  const vk::Format&             imageFormat
) {
  std::vector<vk::ImageView> imageViews;
  imageViews.resize(images.size());

  for (size_t i=0; i < images.size(); i++) {
    imageViews[i] = Excal::Image::createImageView(
      device, images[i], imageFormat, vk::ImageAspectFlagBits::eColor
    );
  }

  return imageViews;
}

// Images can have different layouts that affect how the pixels are stored in memory
// When performing operations on images, you want to transition the image
// to a layout that is optimal for that operation's performance
void transitionImageLayout(
  const vk::Device&      device,
  const vk::CommandPool& commandPool,
  const vk::Queue&       graphicsQueue,
  const vk::Image&       image,
  const vk::Format&      format,
  const vk::ImageLayout& oldLayout,
  const vk::ImageLayout& newLayout
) {
  auto cmd = Excal::Buffer::beginSingleTimeCommands(device, commandPool);

  vk::PipelineStageFlagBits srcPipelineStage;
  vk::PipelineStageFlagBits dstPipelineStage;

  vk::AccessFlagBits srcAccessMask;
  vk::AccessFlagBits dstAccessMask;

  // Specify source and destination pipeline barriers
  if (   oldLayout == vk::ImageLayout::eUndefined
      && newLayout == vk::ImageLayout::eTransferDstOptimal
  ) {
    srcAccessMask = static_cast<vk::AccessFlagBits>(0);
    dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    srcPipelineStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstPipelineStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (   oldLayout == vk::ImageLayout::eTransferDstOptimal
             && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal
  ) {
    srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    dstAccessMask = vk::AccessFlagBits::eShaderRead;

    srcPipelineStage = vk::PipelineStageFlagBits::eTransfer;
    dstPipelineStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vk::ImageMemoryBarrier barrier(
    srcAccessMask, dstAccessMask,
    oldLayout, newLayout,
    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    image,
    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
  );

  cmd.pipelineBarrier(
    srcPipelineStage, dstPipelineStage,
    vk::DependencyFlagBits::eByRegion,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  Excal::Buffer::endSingleTimeCommands(device, cmd, commandPool, graphicsQueue);
}

void copyBufferToImage(
  const vk::Device&      device,
  const vk::CommandPool& commandPool,
  const vk::Queue&       graphicsQueue,
  const vk::Buffer&      buffer,
  const vk::Image&       image,
  const uint32_t         width,
  const uint32_t         height
) {
  auto cmd = Excal::Buffer::beginSingleTimeCommands(device, commandPool);

  // Specify which part of the buffer is to be copied to which part of the image
  vk::BufferImageCopy copyRegion(
    0, 0, 0,
    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
    vk::Offset3D(0, 0, 0),
    vk::Extent3D(width, height, 1)
  );

  cmd.copyBufferToImage(
    buffer, image,
    vk::ImageLayout::eTransferDstOptimal,
    1, &copyRegion
  );

  Excal::Buffer::endSingleTimeCommands(device, cmd, commandPool, graphicsQueue);
}

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
) {
  auto imageCreateInfo = static_cast<VkImageCreateInfo>(
    vk::ImageCreateInfo(
      {}, vk::ImageType::e2D, format,
      vk::Extent3D(width, height, 1), 1, 1,
      numSamples,
      tiling, usage,
      vk::SharingMode::eExclusive
    )
  );

  VkImage image;
  vmaCreateImage(
    allocator, &imageCreateInfo, &allocInfo,
    &image,    &imageAllocation,  nullptr
  );

  return image;
}

ImageResources createColorResources(
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  VmaAllocator&                  allocator,
  const vk::Format&              swapchainImageFormat,
  const vk::Extent2D&            swapchainExtent,
  const vk::SampleCountFlagBits& msaaSamples
) {
  ImageResources colorResources;
  vk::Format colorFormat = swapchainImageFormat;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  colorResources.image = createImage(
    allocator,             colorResources.imageAllocation,
    allocInfo,
    physicalDevice,        device,
    swapchainExtent.width, swapchainExtent.height,
    msaaSamples,           swapchainImageFormat,
    vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eTransientAttachment
    | vk::ImageUsageFlagBits::eColorAttachment,
    vk::MemoryPropertyFlagBits::eDeviceLocal
  );

  colorResources.imageView = createImageView(
    device,
    colorResources.image,
    swapchainImageFormat,
    vk::ImageAspectFlagBits::eColor
  );

  return colorResources;
}

ImageResources createDepthResources(
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  VmaAllocator&                  allocator,
  const vk::Format&              depthFormat,
  const vk::Format&              swapchainImageFormat,
  const vk::Extent2D&            swapchainExtent,
  const vk::SampleCountFlagBits& msaaSamples
) {
  ImageResources depthResources;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  depthResources.image = createImage(
    allocator,             depthResources.imageAllocation,
    allocInfo,
    physicalDevice,        device,
    swapchainExtent.width, swapchainExtent.height,
    msaaSamples,           depthFormat,
    vk::ImageTiling::eOptimal,
    vk::ImageUsageFlagBits::eDepthStencilAttachment,
    vk::MemoryPropertyFlagBits::eDeviceLocal
  );

  depthResources.imageView = createImageView(
    device,
    depthResources.image,
    depthFormat,
    vk::ImageAspectFlagBits::eDepth
  );

  return depthResources;
}

ImageResources createTextureResources(
  const vk::PhysicalDevice& physicalDevice,
  const vk::Device&         device,
  VmaAllocator&             allocator,
  const vk::CommandPool&    commandPool,
  const vk::Queue&          graphicsQueue,
  const std::string&        texturePath
) {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(
    texturePath.c_str(),
    &texWidth, &texHeight, &texChannels,
    STBI_rgb_alpha
  );

  vk::DeviceSize imageSize = texWidth * texHeight * 4;

  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }

  // Staging buffer is on the CPU
  VmaAllocationCreateInfo stagingAllocInfo = {};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VmaAllocation stagingBufferAllocation;

  auto stagingBuffer = Excal::Buffer::createBuffer(
    allocator,      stagingBufferAllocation, stagingAllocInfo,
    physicalDevice, device,                  imageSize,
    vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible
    | vk::MemoryPropertyFlagBits::eHostCoherent
  );

  void* mappedData;
  vmaMapMemory(allocator, stagingBufferAllocation, &mappedData);
  memcpy(mappedData, pixels, (size_t) imageSize);
  vmaUnmapMemory(allocator, stagingBufferAllocation);

  stbi_image_free(pixels);

  ImageResources textureResources;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  textureResources.image = createImage(
    allocator,      textureResources.imageAllocation,
    allocInfo,
    physicalDevice, device,
    texWidth,       texHeight,
    vk::SampleCountFlagBits::e1,  // TODO
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eTransferDst
    | vk::ImageUsageFlagBits::eSampled,
    vk::MemoryPropertyFlagBits::eDeviceLocal
  );

  transitionImageLayout(
    device,
    commandPool,
    graphicsQueue,
    textureResources.image,
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eTransferDstOptimal
  );

  copyBufferToImage(
    device,
    commandPool,
    graphicsQueue,
    stagingBuffer,
    textureResources.image,
    texWidth,
    texHeight
  );

  transitionImageLayout(
    device,
    commandPool,
    graphicsQueue,
    textureResources.image,
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageLayout::eTransferDstOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal
  );

  vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

  textureResources.imageView = createImageView(
    device,
    textureResources.image,
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageAspectFlagBits::eColor
  );

  return textureResources;
}

vk::Sampler createTextureImageSampler(
  const vk::Device& device
) {
  return device.createSampler(
    vk::SamplerCreateInfo(
      // Specify how to interpolate texels that are
      // magnified (oversampling) or minified (undersampling)
      {}, vk::Filter::eLinear, vk::Filter::eLinear,
      vk::SamplerMipmapMode::eLinear,
      // Sampler address mode specifies how to
      // read texels that are outside of the image
      vk::SamplerAddressMode::eRepeat,
      vk::SamplerAddressMode::eRepeat,
      vk::SamplerAddressMode::eRepeat,
      0.0f,
      VK_TRUE, 16.0f, // Anisotropic filtering
      VK_FALSE, vk::CompareOp::eAlways,
      0.0f, 0.0f
    )
  );
}

vk::Format findSupportedFormat(
  const vk::PhysicalDevice&      physicalDevice,
  const std::vector<vk::Format>& candidates,
  const vk::ImageTiling&         tiling,
  const vk::FormatFeatureFlags&  features
) {
  for (auto format : candidates) {
    auto props = physicalDevice.getFormatProperties(format);

    if (   tiling == vk::ImageTiling::eLinear
        && (props.linearTilingFeatures & features) == features
    ) {
      return format;
    } else if (   tiling == vk::ImageTiling::eOptimal
               && (props.optimalTilingFeatures & features) == features
    ) {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}

vk::Format findDepthFormat(
  const vk::PhysicalDevice& physicalDevice
) {
  return findSupportedFormat(
    physicalDevice,
    {
      vk::Format::eD32Sfloat,
      vk::Format::eD32SfloatS8Uint,
      vk::Format::eD24UnormS8Uint
    },
    vk::ImageTiling::eOptimal,
    vk::FormatFeatureFlagBits::eDepthStencilAttachment
  );
}
}
