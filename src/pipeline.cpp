#include "pipeline.h"

#include <vulkan/vulkan.hpp>
#include <fstream>

#include "structs.h"

namespace Excal::Pipeline
{
vk::RenderPass createRenderPass(
  const vk::Device&              device,
  const vk::Format&              depthFormat,
  const vk::Format&              swapchainImageFormat,
  const vk::SampleCountFlagBits& msaaSamples
) {
  vk::AttachmentDescription colorAttachment(
    {}, swapchainImageFormat,
    msaaSamples,
    vk::AttachmentLoadOp::eClear,
    vk::AttachmentStoreOp::eStore,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eColorAttachmentOptimal
  );

  vk::AttachmentDescription depthAttachment(
    {}, depthFormat,
    msaaSamples,
    vk::AttachmentLoadOp::eClear,
    vk::AttachmentStoreOp::eDontCare,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eDepthStencilAttachmentOptimal
  );

  // Resolve the multiple fragments per pixel created via MSAA
  vk::AttachmentDescription colorAttachmentResolve(
    {}, swapchainImageFormat,
    vk::SampleCountFlagBits::e1,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eStore,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::ePresentSrcKHR
  );

  vk::AttachmentReference colorAttachmentRef(
    0, vk::ImageLayout::eColorAttachmentOptimal
  );

  vk::AttachmentReference depthAttachmentRef(
    1, vk::ImageLayout::eDepthStencilAttachmentOptimal
  );

  vk::AttachmentReference colorAttachmentResolveRef(
    2, vk::ImageLayout::eColorAttachmentOptimal
  );

  vk::SubpassDescription subpass(
    {}, vk::PipelineBindPoint::eGraphics, 0,
    nullptr, 1,
    &colorAttachmentRef,
    &colorAttachmentResolveRef,
    &depthAttachmentRef
  );

  vk::SubpassDependency dependency(
    VK_SUBPASS_EXTERNAL, 0,
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::AccessFlagBits::eColorAttachmentWrite,
    vk::AccessFlagBits::eColorAttachmentWrite
  );

  std::array<vk::AttachmentDescription, 3> attachments = {
    colorAttachment,
    depthAttachment,
    colorAttachmentResolve
  };

  return device.createRenderPass(
    vk::RenderPassCreateInfo(
      {}, attachments.size(),
      attachments.data(),
      1, &subpass,
      1, &dependency
    )
  );
}

vk::Pipeline createGraphicsPipeline(
  const vk::Device&              device,
  const vk::PipelineLayout&      pipelineLayout,
  const vk::PipelineCache&       pipelineCache,
  const vk::RenderPass&          renderPass,
  const vk::Extent2D             swapchainExtent,
  const vk::SampleCountFlagBits& msaaSamples,
  const std::string&             vertShaderPath,
  const std::string&             fragShaderPath,
  const std::string&             frontFace
) {
  auto vertShaderModule = createShaderModule(device, vertShaderPath);
  auto fragShaderModule = createShaderModule(device, fragShaderPath);

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
    vk::PipelineShaderStageCreateInfo(
      {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"
    ),
    vk::PipelineShaderStageCreateInfo(
      {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main"
    )
  };

  // Fixed functions of graphics pipeline
  vk::Viewport viewport(
    0.0f, 0.0f,
    (float) swapchainExtent.width,
    (float) swapchainExtent.height,
    0.0f, 1.0f
  );
  vk::Rect2D scissor({0, 0}, swapchainExtent);

  vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
    {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE
  );

  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
    {}, 1, &bindingDescription,
    attributeDescriptions.size(),
    attributeDescriptions.data()
  );

  auto rasterizerFrontFace = frontFace == "clockwise"
                             ? vk::FrontFace::eClockwise
                             : vk::FrontFace::eCounterClockwise;

  vk::PipelineRasterizationStateCreateInfo rasterizer(
    {}, VK_FALSE, VK_FALSE,
    vk::PolygonMode::eFill,
    vk::CullModeFlagBits::eBack,
    rasterizerFrontFace,
    VK_FALSE
  );
  rasterizer.lineWidth = 1.0f;

  vk::PipelineMultisampleStateCreateInfo multisampling(
    {}, msaaSamples,
    VK_FALSE, 1.0f, // Disable sample shading
    nullptr,
    VK_FALSE, VK_FALSE
  );

  vk::PipelineDepthStencilStateCreateInfo depthStencil(
    {}, VK_TRUE, VK_TRUE,
    vk::CompareOp::eLess,
    VK_FALSE, // depthBoundsTestEnable
    VK_FALSE  // stencilTestEnable
  );

  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR
                                      | vk::ColorComponentFlagBits::eG
                                      | vk::ColorComponentFlagBits::eB
                                      | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable = VK_FALSE;

  vk::PipelineColorBlendStateCreateInfo colorBlending(
    {}, VK_FALSE, vk::LogicOp::eClear, 1, &colorBlendAttachment
  );

  std::vector<vk::DynamicState> dynamicStateEnables = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  vk::PipelineDynamicStateCreateInfo dynamicState(
    {}, dynamicStateEnables.size(),
    dynamicStateEnables.data()
  );

  // Combine above structs to create graphics pipeline
  auto graphicsPipeline = device.createGraphicsPipeline(
    pipelineCache,
    vk::GraphicsPipelineCreateInfo(
      {},               shaderStages.size(), shaderStages.data(),
      &vertexInputInfo, &inputAssembly,      nullptr,
      &viewportState,   &rasterizer,         &multisampling,
      &depthStencil,    &colorBlending,      &dynamicState,
      pipelineLayout,   renderPass,          0
    )
  ).value;
  // Calling `.value` is a workaround for a known issue
  // Should be resolved after pull request #678 gets merged
  // https://github.com/KhronosGroup/Vulkan-Hpp/pull/678

  device.destroyShaderModule(vertShaderModule);
  device.destroyShaderModule(fragShaderModule);

  return graphicsPipeline;
}

vk::ShaderModule createShaderModule(
  const vk::Device&  device,
  const std::string& filename
) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t) file.tellg();
  std::vector<char> fileBuffer(fileSize);

  file.seekg(0);
  file.read(fileBuffer.data(), fileSize);
  file.close();

  return device.createShaderModule(
    vk::ShaderModuleCreateInfo(
      {}, fileBuffer.size(),
      reinterpret_cast<const uint32_t*>(fileBuffer.data())
    )
  );
}
}
