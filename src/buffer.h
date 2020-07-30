#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

#include "utils.h"

namespace Excal::Buffer
{
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

void createBuffer(
  vk::Buffer&                    buffer,
  vk::DeviceMemory&              bufferMemory,
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  const vk::DeviceSize&          bufferSize,
  const vk::BufferUsageFlags&    usage,
  const vk::MemoryPropertyFlags& properties
);

// Since template functions are turned into "real functions" at compile time
// they must be defined in the same scope as where they are called from, therefore:
// **Template functions have to be defined in the header**
template <typename T>
void createVkBuffer(
  vk::Buffer&                    buffer,
  vk::DeviceMemory&              bufferMemory,
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  const std::vector<T>&          data,
  const vk::BufferUsageFlagBits& usage,
  const vk::CommandPool&         commandPool,
  const vk::Queue&               cmdQueue
) {
  vk::DeviceSize bufferSize = sizeof(data[0]) * data.size();

  // Staging buffer is on the CPU
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;

  createBuffer(
    stagingBuffer,
    stagingBufferMemory,
    physicalDevice,
    device,
    bufferSize,
    vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible
    | vk::MemoryPropertyFlagBits::eHostCoherent
  );

  void* memData = device.mapMemory(stagingBufferMemory, 0, bufferSize);
  memcpy(memData, data.data(), (size_t) bufferSize);
  device.unmapMemory(stagingBufferMemory);

  // Create buffer on the GPU (device visible)
  createBuffer(
    buffer,
    bufferMemory,
    physicalDevice,
    device,
    bufferSize,
    vk::BufferUsageFlagBits::eTransferDst | usage,
    vk::MemoryPropertyFlagBits::eDeviceLocal
  );

  // Copy host visible staging buffer to device visible buffer
  auto cmd = beginSingleTimeCommands(device, commandPool);

  auto copyRegion = vk::BufferCopy(0, 0, bufferSize);
  cmd.copyBuffer(stagingBuffer, buffer, 1, &copyRegion);

  endSingleTimeCommands(device, cmd, commandPool, cmdQueue);

  // Free resources
  device.destroyBuffer(stagingBuffer);
  device.freeMemory(stagingBufferMemory);
}
}
