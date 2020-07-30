#include "buffer.h"

#include <vulkan/vulkan.hpp>
#include <vector>

#include "utils.h"

namespace Excal::Buffer
{
void createBuffer(
  vk::Buffer&                    buffer,
  vk::DeviceMemory&              bufferMemory,
  const vk::PhysicalDevice&      physicalDevice,
  const vk::Device&              device,
  const vk::DeviceSize&          bufferSize,
  const vk::BufferUsageFlags&    usage,
  const vk::MemoryPropertyFlags& properties
) {
  buffer = device.createBuffer(
    vk::BufferCreateInfo({}, bufferSize, usage, vk::SharingMode::eExclusive)
  );

  auto memRequirements = device.getBufferMemoryRequirements(buffer);

  bufferMemory = device.allocateMemory(
    vk::MemoryAllocateInfo(
      memRequirements.size,
      Excal::Utils::findMemoryType(
        physicalDevice, memRequirements.memoryTypeBits, properties
      )
    )
  );

  device.bindBufferMemory(buffer, bufferMemory, 0);
}

vk::CommandBuffer beginSingleTimeCommands(
  const vk::Device&      device,
  const vk::CommandPool& commandPool
) {
  auto commandBuffers = device.allocateCommandBuffers(
    vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1)
  );

  vk::CommandBuffer& cmd = commandBuffers[0];

  cmd.begin(
    // eOneTimeSubmit specifies that each recording of the command buffer will
    // only be submitted once, and then reset and recorded again between submissions
    vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
  );

  return cmd;
}

void endSingleTimeCommands(
  const vk::Device&        device,
  const vk::CommandBuffer& cmd,
  const vk::CommandPool&   commandPool,
  const vk::Queue&         cmdQueue
) {
  cmd.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  cmdQueue.submit(1, &submitInfo, nullptr);
  cmdQueue.waitIdle();

  device.freeCommandBuffers(commandPool, 1, &cmd);
}
}
