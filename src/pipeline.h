#pragma once

#include <vulkan/vulkan.hpp>

namespace Excal::Pipeline {
vk::RenderPass createRenderPass(
  const vk::Device&              device,
  const vk::Format&              depthFormat,
  const vk::Format&              swapchainImageFormat,
  const vk::SampleCountFlagBits& msaaSamples
);

vk::Pipeline createGraphicsPipeline(
  const vk::Device&              device,
  const vk::PipelineLayout&      pipelineLayout,
  const vk::PipelineCache&       pipelineCache,
  const vk::RenderPass&          renderPass,
  const vk::Extent2D             swapchainExtent,
  const vk::SampleCountFlagBits& msaaSamples
);

vk::ShaderModule createShaderModule(
  const vk::Device&  device,
  const std::string& filename
);
}
