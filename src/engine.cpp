#include "engine.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <array>

#include "structs.h"
#include "device.h"
#include "surface.h"
#include "debug.h"
#include "swapchain.h"
#include "model.h"
#include "buffer.h"
#include "image.h"
#include "pipeline.h"
#include "descriptor.h"
#include "utils.h"

namespace Excal
{
Engine::Engine() {}

Engine::~Engine()
{
  cleanup();
}

void Engine::initVulkan()
{
  // Debugger setup
  const bool validationLayersSupported = Excal::Debug::checkValidationLayerSupport();
  const auto validationLayers          = Excal::Debug::getValidationLayers();
  const auto debugMessengerCreateInfo  = Excal::Debug::getDebugMessengerCreateInfo();

  window = Excal::Surface::initWindow(
    &framebufferResized,
    config.windowWidth,
    config.windowHeight,
    config.appName.c_str()
  );

  vk::ApplicationInfo appInfo(
    config.appName.c_str(),
    config.appVersion,
    "Excal",
    1.0,
    VK_API_VERSION_1_2
  );

  instance = Excal::Device::createInstance(
    appInfo,
    validationLayersEnabled,
    validationLayersSupported,
    validationLayers,
    debugMessengerCreateInfo
  );

  debugMessenger = Excal::Debug::setupDebugMessenger(
    instance, validationLayersEnabled, debugMessengerCreateInfo
  );

  surface = Excal::Surface::createSurface(instance, window);

  physicalDevice          = Excal::Device::pickPhysicalDevice(instance, surface);
  auto queueFamilyIndices = Excal::Device::findQueueFamilies(physicalDevice, surface);

  device = Excal::Device::createLogicalDevice(physicalDevice, queueFamilyIndices);

  graphicsQueue = device.getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
  presentQueue  = device.getQueue(queueFamilyIndices.presentFamily.value(), 0);

  msaaSamples = Excal::Device::getMaxUsableSampleCount(physicalDevice);

  // Initalize Vulkan Memory Allocator
  VmaAllocatorCreateInfo allocatorInfo = {};

  allocatorInfo.physicalDevice = physicalDevice;
  allocatorInfo.device         = device;
  allocatorInfo.instance       = instance;

  vmaCreateAllocator(&allocatorInfo, &allocator);

  // Create sync objects for each frame in flight
  imageAvailableSemaphores.resize(config.maxFramesInFlight);
  renderFinishedSemaphores.resize(config.maxFramesInFlight);
  inFlightFences.resize(config.maxFramesInFlight);
  imagesInFlight.resize(config.maxFramesInFlight);

  for (int i=0; i < config.maxFramesInFlight; i++)
  {
    imageAvailableSemaphores[i] = device.createSemaphore({}, nullptr);
    renderFinishedSemaphores[i] = device.createSemaphore({}, nullptr);
    inFlightFences[i] = device.createFence(
      vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled), nullptr
    );
  }

  descriptorSetLayout = Excal::Descriptor::createDescriptorSetLayout(
    device, config.models.size()
  );

  commandPool = device.createCommandPool(
    vk::CommandPoolCreateInfo({}, queueFamilyIndices.graphicsFamily.value())
  );

  // Create texture resources for each model
  for (auto& model : config.models) {
    textures.push_back(
      Excal::Image::createTextureResources(
        physicalDevice, device,
        allocator,      commandPool,
        graphicsQueue,  model.texturePath
      )
    );
  }

  for (auto& texture : textures) {
    textureImageViews.push_back(texture.imageView);
  }

  textureSampler = Excal::Image::createTextureImageSampler(device);

  // Create vectors containing all model indices and vertices
  std::vector<uint32_t> indices;
  std::vector<Vertex>   vertices;

  for (auto& model : config.models) {
    indexCounts.push_back(model.indices.size());
    vertexCounts.push_back(model.vertices.size());
    indices.insert( indices.end(),  model.indices.begin(),  model.indices.end());
    vertices.insert(vertices.end(), model.vertices.begin(), model.vertices.end());
  }

  // Create buffers with VMA
  // Create single index buffer for all models
  indexBuffer = Excal::Buffer::createVkBuffer(
    allocator,      indexBufferAllocation,
    physicalDevice, device,
    indices,        commandPool,
    graphicsQueue,
    vk::BufferUsageFlagBits::eIndexBuffer
  );

  // Create single vertex buffer for all models
  vertexBuffer = Excal::Buffer::createVkBuffer(
    allocator,      vertexBufferAllocation,
    physicalDevice, device,
    vertices,       commandPool,
    graphicsQueue,
    vk::BufferUsageFlagBits::eVertexBuffer
  );

  // Set alignment for dynamic uniform buffers
  auto deviceProps = physicalDevice.getProperties();
  size_t minUboAlignment = deviceProps.limits.minUniformBufferOffsetAlignment;

  dynamicAlignment = sizeof(DynamicUniformBufferObject);
  if (minUboAlignment > 0) {
    dynamicAlignment = (dynamicAlignment + minUboAlignment - 1)
                       & ~(minUboAlignment - 1);
  }

  createSwapchainObjects();
}

void Engine::createSwapchainObjects()
{
  auto queueFamilyIndices = Excal::Device::findQueueFamilies(physicalDevice, surface);

  // Create swapchain and image views
  auto swapchainState = Excal::Swapchain::createSwapchain(
    physicalDevice, device,
    surface, window,
    queueFamilyIndices
  );

  swapchain            = swapchainState.swapchain;
  swapchainImageFormat = swapchainState.swapchainImageFormat;
  swapchainExtent      = swapchainState.swapchainExtent;
  swapchainImages      = device.getSwapchainImagesKHR(swapchain);

  swapchainImageViews = Excal::Image::createImageViews(
    device, swapchainImages, swapchainImageFormat
  );

  depthFormat = Excal::Image::findDepthFormat(physicalDevice);

  renderPass = Excal::Pipeline::createRenderPass(
    device,
    depthFormat,
    swapchainImageFormat,
    msaaSamples
  );

  vk::PushConstantRange pushConstantRange(
    vk::ShaderStageFlagBits::eFragment, 0, sizeof(int)
  );

  pipelineLayout = device.createPipelineLayout(
    vk::PipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 1, &pushConstantRange)
  );

  // Read pipeline cache data from disk
  // If file for pipeline cach doesn't exist, cacheFileSize will be 0
  // which causes pipelineCache to not set initialData (i.e. no errors!)
  void* pCacheData;
  auto cacheFile = std::fstream("pipeline-cache", std::ios::out | std::ios::binary);
  auto cacheFileSize = cacheFile.tellg();
  cacheFile.read((char*) pCacheData, cacheFileSize);

  pipelineCache = device.createPipelineCache(
    vk::PipelineCacheCreateInfo({}, cacheFileSize, pCacheData)
  );

  cacheFile.close();

  graphicsPipeline = Excal::Pipeline::createGraphicsPipeline(
    device,                pipelineLayout,
    pipelineCache,         renderPass,
    swapchainExtent,       msaaSamples,
    config.vertShaderPath, config.fragShaderPath
  );

  // Create resources
  colorResources = Excal::Image::createColorResources(
    physicalDevice,  device,
    allocator,       swapchainImageFormat,
    swapchainExtent, msaaSamples
  );

  depthResources = Excal::Image::createDepthResources(
    physicalDevice,       device,
    allocator,            depthFormat,
    swapchainImageFormat, swapchainExtent,
    msaaSamples
  );

  swapchainFramebuffers = Excal::Buffer::createFramebuffers(
    device,                   swapchainImageViews,
    colorResources.imageView, depthResources.imageView,
    renderPass,               swapchainExtent
  );

  uniformBufferAllocations.resize(swapchainImageViews.size());
  dynamicUniformBufferAllocations.resize(swapchainImageViews.size());

  uniformBuffers = Excal::Buffer::createUniformBuffers(
    physicalDevice, device,
    allocator,      uniformBufferAllocations
  );

  dynamicUniformBuffers = Excal::Buffer::createDynamicUniformBuffers(
    uboDynamicData, dynamicAlignment,
    physicalDevice, device,
    allocator,      dynamicUniformBufferAllocations,
    config.models.size()
  );

  descriptorPool = Excal::Descriptor::createDescriptorPool(
    device, swapchainImages.size(), textures.size()
  );

  descriptorSets = Excal::Descriptor::createDescriptorSets(
    device,            swapchainImages.size(),
    descriptorPool,    descriptorSetLayout,
    uniformBuffers,    dynamicUniformBuffers,
    textureImageViews, textureSampler
  );

  commandBuffers = Excal::Buffer::createCommandBuffers(
    device,                commandPool,
    swapchainFramebuffers, swapchainExtent,
    graphicsPipeline,      pipelineLayout,
    indexCounts,           vertexCounts,
    indexBuffer,           vertexBuffer,
    renderPass,            descriptorSets,
    dynamicAlignment
  );
}

void Engine::mainLoop()
{
  size_t currentFrame = 0;

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    drawFrame(currentFrame);
  }

  device.waitIdle();
}

void Engine::drawFrame(size_t& currentFrame)
{
  device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  vk::Result result = device.acquireNextImageKHR(
    swapchain, UINT64_MAX,
    imageAvailableSemaphores[currentFrame],
    nullptr, &imageIndex
  );

  // Usually happens after a window resize
  if (   result == vk::Result::eErrorOutOfDateKHR
      || result == vk::Result::eSuboptimalKHR
  ) {
    recreateSwapchain();
    return;
  }

  Excal::Buffer::updateUniformBuffer(
    allocator, uniformBufferAllocations,
    device,    swapchainExtent,
    imageIndex
  );

  Excal::Buffer::updateDynamicUniformBuffer(
    uboDynamicData, dynamicAlignment,
    allocator,      dynamicUniformBufferAllocations,
    device,         swapchainExtent,
    imageIndex,     config.models
  );

  // Check if a previous frame is using this image
  // (i.e. there is its fence to wait on)
  if (imagesInFlight[imageIndex]) {
    device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  }
  // Mark the image as now being in use by this frame
  imagesInFlight[imageIndex] = inFlightFences[currentFrame];

  vk::Semaphore signalSemaphores[]    = {renderFinishedSemaphores[currentFrame]};
  vk::Semaphore waitSemaphores[]      = {imageAvailableSemaphores[currentFrame]};
  vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

  vk::SubmitInfo submitInfo(
    1, waitSemaphores, waitStages,
    1, &commandBuffers[imageIndex],
    1, signalSemaphores
  );

  device.resetFences(1, &inFlightFences[currentFrame]);

  graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);

  vk::SwapchainKHR swapchains[] = {swapchain};

  result = presentQueue.presentKHR(
    vk::PresentInfoKHR(1, signalSemaphores, 1, swapchains, &imageIndex)
  );

  if (   result == vk::Result::eErrorOutOfDateKHR
      || result == vk::Result::eSuboptimalKHR
      || framebufferResized
  ) {
    framebufferResized = false;
    recreateSwapchain();
    return;
  }

  currentFrame = (currentFrame + 1) % config.maxFramesInFlight;
}

void Engine::recreateSwapchain()
{
  // Handle widow minimization
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);

  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  device.waitIdle();

  cleanupSwapchain();
  createSwapchainObjects();
}

void Engine::cleanupSwapchain()
{
  device.destroyImageView(colorResources.imageView);
  device.destroyImageView(depthResources.imageView);

  vmaDestroyImage(allocator, colorResources.image, colorResources.imageAllocation);
  vmaDestroyImage(allocator, depthResources.image, depthResources.imageAllocation);

  for (auto framebuffer : swapchainFramebuffers) {
    device.destroyFramebuffer(framebuffer);
  }

  device.freeCommandBuffers(commandPool, commandBuffers);

  device.destroyDescriptorPool(descriptorPool);

  device.destroyPipeline(graphicsPipeline);
  device.destroyPipelineCache(pipelineCache);
  device.destroyPipelineLayout(pipelineLayout);
  device.destroyRenderPass(renderPass);

  for (size_t i=0; i < swapchainImageViews.size(); i++) {
    device.destroyImageView(swapchainImageViews[i]);
    vmaDestroyBuffer(allocator, uniformBuffers[i], uniformBufferAllocations[i]);
    vmaDestroyBuffer(
      allocator, dynamicUniformBuffers[i], dynamicUniformBufferAllocations[i]
    );
  }

  device.destroySwapchainKHR(swapchain);
}

void Engine::cleanup()
{
  // Write pipeline cache data to disk
  auto pipelineCacheData = device.getPipelineCacheData(pipelineCache);

  auto cacheFile = std::fstream("pipeline-cache", std::ios::out | std::ios::binary);

  cacheFile.write((char*) &pipelineCacheData[0], sizeof(pipelineCacheData));
  cacheFile.close();

  cleanupSwapchain();

  for (int i=0; i < config.maxFramesInFlight; i++) {
    device.destroySemaphore(imageAvailableSemaphores[i]);
    device.destroySemaphore(renderFinishedSemaphores[i]);
    device.destroyFence(inFlightFences[i]);
  }

  device.destroyDescriptorSetLayout(descriptorSetLayout);

  Excal::Utils::alignedFree(uboDynamicData.model);

  device.destroySampler(textureSampler);

  for (int i=0; i < textures.size(); i++) {
    device.destroyImageView(textures[i].imageView);
    vmaDestroyImage(allocator, textures[i].image, textures[i].imageAllocation);
  }

  vmaDestroyBuffer(allocator, indexBuffer, indexBufferAllocation);
  vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);

  vmaDestroyAllocator(allocator);

  device.destroyCommandPool(commandPool);
  device.destroy();

  if (validationLayersEnabled)
  {
    Excal::Debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  instance.destroySurfaceKHR(surface);
  instance.destroy();

  glfwDestroyWindow(window);
  glfwTerminate();
}

int Engine::run()
{
  try {
    mainLoop();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
}
