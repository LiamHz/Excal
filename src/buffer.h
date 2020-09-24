#pragma once

#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <vector>

#include "structs.h"
#include "model.h"
#include "camera.h"

namespace Excal::Buffer
{
vk::Buffer createBuffer(
  VmaAllocator&                  allocator,
  VmaAllocation&                 bufferAllocation,
  VmaAllocationCreateInfo&       allocInfo,
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  const vk::DeviceSize&          bufferSize,
  const vk::BufferUsageFlags&    usage,
  const vk::MemoryPropertyFlags& properties
);

std::vector<vk::CommandBuffer> createCommandBuffers(
  const vk::Device&                     device,
  const vk::CommandPool&                commandPool,
  const std::vector<VkFramebuffer>&     swapchainFramebuffers,
  const vk::Extent2D                    swapchainExtent,
  const vk::Pipeline&                   graphicsPipeline,
  const vk::PipelineLayout&             pipelineLayout,
  const std::vector<uint32_t>&          indexCounts,
  const std::vector<uint32_t>&          vertexCounts,
  const vk::Buffer&                     indexBuffer,
  const vk::Buffer&                     vertexBuffer,
  const vk::RenderPass&                 renderPass,
  const std::vector<vk::DescriptorSet>& descriptorSets,
  const size_t                          dynamicAlignment,
  const glm::vec4&                      clearColor
);

std::vector<VkFramebuffer> createFramebuffers(
  const vk::Device&                 device,
  const std::vector<vk::ImageView>& swapchainImageViews,
  const vk::ImageView&              colorImageView,
  const vk::ImageView&              depthImageView,
  const vk::RenderPass&             renderPass,
  const vk::Extent2D&               swapchainExtent
);

std::vector<vk::Buffer> createUniformBuffers(
  const vk::PhysicalDevice&   physicalDevice,
  const vk::Device&           device,
  VmaAllocator&               allocator,
  std::vector<VmaAllocation>& bufferAllocations
);

std::vector<vk::Buffer> createDynamicUniformBuffers(
  UboDynamicData&             uboDynamicData,
  const size_t                dynamicAlignment,
  const vk::PhysicalDevice&   physicalDevice,
  const vk::Device&           device,
  VmaAllocator&               allocator,
  std::vector<VmaAllocation>& bufferAllocations,
  const int                   nObjects
);

void updateUniformBuffer(
  VmaAllocator&               allocator,
  std::vector<VmaAllocation>& bufferAllocations,
  const vk::Device&           device,
  const vk::Extent2D&         swapchainExtent,
  const uint32_t              currentImage,
  const float                 farClipPlane,
  const Excal::Camera&        camera
);

void updateDynamicUniformBuffer(
  UboDynamicData&              uboDynamicData,
  const size_t                 dynamicAlignment,
  VmaAllocator&                allocator,
  std::vector<VmaAllocation>&  bufferAllocations,
  const vk::Device&            device,
  const vk::Extent2D&          swapchainExtent,
  const uint32_t               currentImage,
  const std::vector<Excal::Model::Model>& models
);

vk::CommandBuffer beginSingleTimeCommands(
  const vk::Device&      device,
  const vk::CommandPool& commandPool
);

void endSingleTimeCommands(
  const vk::Device&        device,
  const vk::CommandBuffer& cmd,
  const vk::CommandPool&   commandPool,
  const vk::Queue&         cmdQueue
);

// Since template functions are turned into "real functions" at compile time
// they must be defined in the same scope as where they are called from, therefore:
// **Template functions in namespaces have to be defined in the header file**
template <typename T>
vk::Buffer createVkBuffer(
  VmaAllocator&                  allocator,
  VmaAllocation&                 bufferAllocation,
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  const std::vector<T>&          data,
  const vk::CommandPool&         commandPool,
  const vk::Queue&               cmdQueue,
  const vk::BufferUsageFlagBits& usage
) {
  vk::DeviceSize bufferSize = sizeof(data[0]) * data.size();

  // Staging buffer is on the CPU
  VmaAllocationCreateInfo stagingAllocInfo = {};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VmaAllocation stagingBufferAllocation;

  auto stagingBuffer = Excal::Buffer::createBuffer(
    allocator,      stagingBufferAllocation, stagingAllocInfo,
    physicalDevice, device,                  bufferSize,
    vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible
    | vk::MemoryPropertyFlagBits::eHostCoherent
  );

  void* mappedData;
  vmaMapMemory(allocator, stagingBufferAllocation, &mappedData);
  memcpy(mappedData, data.data(), (size_t) bufferSize);
  vmaUnmapMemory(allocator, stagingBufferAllocation);

  // Create buffer on the GPU (device visible)
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  auto buffer = createBuffer(
    allocator,      bufferAllocation, allocInfo,
    physicalDevice, device,           bufferSize,
    vk::BufferUsageFlagBits::eTransferDst | usage,
    vk::MemoryPropertyFlagBits::eDeviceLocal
  );

  // Copy host visible staging buffer to device visible buffer
  auto cmd = beginSingleTimeCommands(device, commandPool);

  auto copyRegion = vk::BufferCopy(0, 0, bufferSize);
  cmd.copyBuffer(stagingBuffer, buffer, 1, &copyRegion);

  endSingleTimeCommands(device, cmd, commandPool, cmdQueue);

  vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

  return buffer;
}
}
