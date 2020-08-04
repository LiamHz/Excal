#include "buffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <vector>
#include <chrono>

#include "device.h"
#include "structs.h"
#include "model.h"

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
) {
  auto bufferCreateInfo = static_cast<VkBufferCreateInfo>(
    vk::BufferCreateInfo(
      {}, bufferSize, usage, vk::SharingMode::eExclusive
    )
  );

  VkBuffer buffer;
  vmaCreateBuffer(
    allocator, &bufferCreateInfo, &allocInfo,
    &buffer,   &bufferAllocation, nullptr
  );

  return buffer;
}

std::vector<vk::CommandBuffer> createCommandBuffers(
  const vk::Device&                     device,
  const vk::CommandPool&                commandPool,
  const std::vector<VkFramebuffer>&     swapchainFramebuffers,
  const vk::Extent2D                    swapchainExtent,
  const vk::Pipeline&                   graphicsPipeline,
  const vk::PipelineLayout&             pipelineLayout,
  const std::vector<uint32_t>&          indexCounts,
  const vk::Buffer&                     indexBuffer,
  const vk::Buffer&                     vertexBuffer,
  const vk::RenderPass&                 renderPass,
  const std::vector<vk::DescriptorSet>& descriptorSets
) {
  std::vector<vk::CommandBuffer> commandBuffers(swapchainFramebuffers.size());

  commandBuffers = device.allocateCommandBuffers(
    vk::CommandBufferAllocateInfo(
      commandPool,
      vk::CommandBufferLevel::ePrimary,
      commandBuffers.size()
    )
  );

  for (size_t i=0; i < commandBuffers.size(); i++) {
    vk::CommandBuffer& cmd = commandBuffers[i];

    std::array<vk::ClearValue, 2> clearValues{
      vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}),
      vk::ClearDepthStencilValue(1.0f, 0)
    };

    cmd.begin(vk::CommandBufferBeginInfo());

    cmd.beginRenderPass(
      vk::RenderPassBeginInfo(
        renderPass,
        swapchainFramebuffers[i],
        vk::Rect2D({0, 0}, swapchainExtent),
        clearValues.size(), clearValues.data()
      ),
      vk::SubpassContents::eInline
    );

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    vk::DeviceSize offsets[] = {0};

    cmd.bindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmd.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

    cmd.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipelineLayout, 0, 1,
      &descriptorSets[i], 0, nullptr
    );

    uint32_t offset = 0;

    for (auto& indexCount : indexCounts) {
      cmd.drawIndexed(indexCount, 1, offset, 0, 0);
      offset += indexCount;
    }

    cmd.endRenderPass();
    cmd.end();
  }

  return commandBuffers;
}

std::vector<VkFramebuffer> createFramebuffers(
  const vk::Device&                 device,
  const std::vector<vk::ImageView>& swapchainImageViews,
  const vk::ImageView&              colorImageView,
  const vk::ImageView&              depthImageView,
  const vk::RenderPass&             renderPass,
  const vk::Extent2D&               swapchainExtent
) {
  // TODO Change this to vk::Framebuffer
  std::vector<VkFramebuffer> swapchainFramebuffers(swapchainImageViews.size());

  for (size_t i=0; i < swapchainImageViews.size(); i++) {
    std::array<vk::ImageView, 3> attachments = {
      colorImageView,
      depthImageView,
      swapchainImageViews[i],
    };

    swapchainFramebuffers[i] = device.createFramebuffer(
      vk::FramebufferCreateInfo(
        {}, renderPass,
        attachments.size(), attachments.data(),
        swapchainExtent.width, swapchainExtent.height, 1
      )
    );
  }

  return swapchainFramebuffers;
}

std::vector<vk::Buffer> createUniformBuffers(
  const vk::PhysicalDevice&   physicalDevice,
  const vk::Device&           device,
  VmaAllocator&               allocator,
  std::vector<VmaAllocation>& bufferAllocations
) {
  vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

  std::vector<vk::Buffer> uniformBuffers(bufferAllocations.size());

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  for (size_t i=0; i < bufferAllocations.size(); i++) {
    uniformBuffers[i] = Excal::Buffer::createBuffer(
      allocator,      bufferAllocations[i], allocInfo,
      physicalDevice, device,               bufferSize,
      vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible
      | vk::MemoryPropertyFlagBits::eHostCoherent
    );
  }

  return uniformBuffers;
}

void updateUniformBuffer(
  VmaAllocator&               allocator,
  std::vector<VmaAllocation>& uniformBufferAllocations,
  const vk::Device&           device,
  const vk::Extent2D&         swapchainExtent,
  const uint32_t              currentImage
) {
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime      = std::chrono::high_resolution_clock::now();

  float time = std::chrono::duration<float, std::chrono::seconds::period>(
    currentTime - startTime
  ).count();

  UniformBufferObject ubo{};

  glm::mat4 uboRotation = glm::rotate(
    glm::mat4(1.0f),
    (1.0f + time / 2.0f) * glm::radians(90.0f),
    glm::vec3(0.0f, 1.0f, 0.0f) // Rotation axis
  );

  glm::mat4 uboTranslation = glm::translate(
    glm::mat4(1.0f),
    glm::vec3(0.0f, -0.67f, 0.0f)
  );

  ubo.model = uboRotation * uboTranslation;

  ubo.view = glm::lookAt(
    glm::vec3(0.0f, 1.0f, 5.0f), // Eye position
    glm::vec3(0.0f),             // Center position
    glm::vec3(0.0f, 1.0f, 0.0f)  // Up Axis
  );

  ubo.proj = glm::perspective(
    glm::radians(45.0f),
    swapchainExtent.width / (float) swapchainExtent.height,
    0.1f, 10.0f
  );

  // Invert Y axis to acccount for difference between OpenGL and Vulkan
  ubo.proj[1][1] *= -1;

  void* mappedData;
  vmaMapMemory(allocator, uniformBufferAllocations[currentImage], &mappedData);
  memcpy(mappedData, &ubo, sizeof(ubo));
  vmaUnmapMemory(allocator, uniformBufferAllocations[currentImage]);
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
