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
#include <math.h>

#include "device.h"
#include "structs.h"
#include "model.h"
#include "utils.h"

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
  const std::vector<uint32_t>&          vertexCounts,
  const vk::Buffer&                     indexBuffer,
  const vk::Buffer&                     vertexBuffer,
  const vk::RenderPass&                 renderPass,
  const std::vector<vk::DescriptorSet>& descriptorSets,
  const size_t                          dynamicAlignment,
  const glm::vec4&                      clearColor
) {
  std::vector<vk::CommandBuffer> commandBuffers(swapchainFramebuffers.size());

  commandBuffers = device.allocateCommandBuffers(
    vk::CommandBufferAllocateInfo(
      commandPool,
      vk::CommandBufferLevel::ePrimary,
      commandBuffers.size()
    )
  );

  for (size_t nCmdBuffer=0; nCmdBuffer < commandBuffers.size(); nCmdBuffer++) {
    vk::CommandBuffer& cmd = commandBuffers[nCmdBuffer];

    std::array<vk::ClearValue, 2> clearValues{
      vk::ClearColorValue(std::array<float, 4>{
        clearColor.r, clearColor.g, clearColor.b, clearColor.a,
      }),
      vk::ClearDepthStencilValue(1.0f, 0)
    };

    cmd.begin(vk::CommandBufferBeginInfo());

    cmd.beginRenderPass(
      vk::RenderPassBeginInfo(
        renderPass,
        swapchainFramebuffers[nCmdBuffer],
        vk::Rect2D({0, 0}, swapchainExtent),
        clearValues.size(), clearValues.data()
      ),
      vk::SubpassContents::eInline
    );

    vk::Viewport viewport(
      0.0f, 0.0f,
      (float) swapchainExtent.width,
      (float) swapchainExtent.height,
      0.0f, 1.0f
    );
    vk::Rect2D scissor({0, 0}, swapchainExtent);

    cmd.setViewport(0, 1, &viewport);
    cmd.setScissor(0, 1, &scissor);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    vk::DeviceSize offsets[] = {0};

    cmd.bindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmd.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

    uint32_t firstIndex   = 0;
    uint32_t vertexOffset = 0;

    for (int i=0; i < indexCounts.size(); i++) {
      // Dynamic descriptor
      uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);

      cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipelineLayout, 0, 1,
        &descriptorSets[nCmdBuffer],
        1, &dynamicOffset
      );

      // Push constant corresponds to index of texture array for current model
      cmd.pushConstants(
        pipelineLayout, vk::ShaderStageFlagBits::eFragment,
        0, sizeof(int), &i
      );

      cmd.drawIndexed(indexCounts[i], 1, firstIndex, vertexOffset, 0);
      firstIndex   += indexCounts[i];
      vertexOffset += vertexCounts[i];
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

std::vector<vk::Buffer> createDynamicUniformBuffers(
  UboDynamicData&             uboDynamicData,
  const size_t                dynamicAlignment,
  const vk::PhysicalDevice&   physicalDevice,
  const vk::Device&           device,
  VmaAllocator&               allocator,
  std::vector<VmaAllocation>& bufferAllocations,
  const int                   nObjects
) {
  size_t bufferSize = nObjects * dynamicAlignment;

  uboDynamicData.model
    = (glm::mat4*)Excal::Utils::alignedAlloc(bufferSize, dynamicAlignment);

  std::vector<vk::Buffer> dynamicUniformBuffers(bufferAllocations.size());

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  /*
  allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
  allocInfo.requiredFlags  = static_cast<VkMemoryPropertyFlags>(
    vk::MemoryPropertyFlagBits::eHostVisible
  );
  allocInfo.preferredFlags = static_cast<VkMemoryPropertyFlags> (
      vk::MemoryPropertyFlagBits::eHostCoherent
    | vk::MemoryPropertyFlagBits::eHostCached
  );
  */

  for (size_t i=0; i < bufferAllocations.size(); i++) {
    dynamicUniformBuffers[i] = Excal::Buffer::createBuffer(
      allocator,      bufferAllocations[i], allocInfo,
      physicalDevice, device,               bufferSize,
      vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible
      | vk::MemoryPropertyFlagBits::eHostCoherent
    );
  }

  return dynamicUniformBuffers;
}

void updateUniformBuffer(
  VmaAllocator&               allocator,
  std::vector<VmaAllocation>& bufferAllocations,
  const vk::Device&           device,
  const vk::Extent2D&         swapchainExtent,
  const uint32_t              currentImage,
  const float                 farClipPlane,
  const float                 cameraMovmentLength,
  const glm::vec3&            cameraStartPos,
  const glm::vec3&            cameraEndPos,
  const glm::vec3&            cameraStartLookAt,
  const glm::vec3&            cameraEndLookAt
) {
  UniformBufferObject ubo{};
  auto cameraPos    = cameraStartPos;
  auto cameraLookAt = cameraStartLookAt;

  // Automatic animation of camera position and look at
  if (cameraStartPos != cameraEndPos || cameraStartLookAt != cameraEndLookAt)
  {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime      = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(
      currentTime - startTime
    ).count();

    // NOTE: cameraMovmentLength is the time it takes for the camera to move from
    //       cameraStartPos to cameraEndPos
    float t = std::fmod(time, cameraMovmentLength) / cameraMovmentLength;

    // Lerp cameraPos and cameraLookAt
    cameraPos = std::fmod(time, cameraMovmentLength*2.0) >= cameraMovmentLength
              ? cameraStartPos * t + cameraEndPos * (1.0f - t)
              : cameraStartPos * (1.0f - t) + cameraEndPos * t;

    cameraLookAt = std::fmod(time, cameraMovmentLength*2.0) >= cameraMovmentLength
                 ? cameraStartLookAt * t + cameraEndLookAt * (1.0f - t)
                 : cameraStartLookAt * (1.0f - t) + cameraEndLookAt * t;
  }

  ubo.view = glm::lookAt(
    cameraPos,
    cameraLookAt,
    glm::vec3(0.0f, 1.0f, 0.0f) // Up Axis
  );

  ubo.proj = glm::perspective(
    glm::radians(45.0f),
    swapchainExtent.width / (float) swapchainExtent.height,
    0.1f, farClipPlane
  );

  // Invert Y axis to acccount for difference between OpenGL and Vulkan
  ubo.proj[1][1] *= -1;

  // TODO Don't map and unmap data every frame. Refer to other TODO in this file
  void* mappedData;
  vmaMapMemory(allocator, bufferAllocations[currentImage], &mappedData);
  memcpy(mappedData, &ubo, sizeof(ubo));
  vmaUnmapMemory(allocator, bufferAllocations[currentImage]);
}

void updateDynamicUniformBuffer(
  UboDynamicData&                    uboDynamicData,
  const size_t                       dynamicAlignment,
  VmaAllocator&                      allocator,
  std::vector<VmaAllocation>&        bufferAllocations,
  const vk::Device&                  device,
  const vk::Extent2D&                swapchainExtent,
  const uint32_t                     currentImage,
  const std::vector<Excal::Model::Model>& models
) {
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime      = std::chrono::high_resolution_clock::now();

  float time = std::chrono::duration<float, std::chrono::seconds::period>(
    currentTime - startTime
  ).count();

  for (uint32_t i=0; i < models.size(); i++) {
    glm::mat4* modelMat = (glm::mat4*)(
      ((uint64_t) uboDynamicData.model + (i * dynamicAlignment))
    );

    auto pos = models[i].position;

    *modelMat = glm::translate(glm::mat4(1.0f), pos);
    *modelMat = glm::scale(*modelMat, glm::vec3(models[i].scale));

    *modelMat = glm::rotate(
      *modelMat,
      (models[i].rotationsPerSecond * time) * glm::radians(360.0f),
      glm::vec3(0.0f, 1.0f, 0.0f) // Rotation axis
    );
  }

  // TODO Mapping and unmapping data every frame just to change a matrix is inefficient
  //      Figure out how to map this data once, possibly with vkFlushMappedMemoryRanges
  //      or the VMA equivalent. Refer to bottom of Sascha Willem's guide
  //      https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
  void* mappedData;
  vmaMapMemory(allocator, bufferAllocations[currentImage], &mappedData);
  memcpy(mappedData, uboDynamicData.model, dynamicAlignment * models.size());
  vmaUnmapMemory(allocator, bufferAllocations[currentImage]);
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
