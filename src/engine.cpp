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
#include "frame.h"

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

  depthFormat = Excal::Image::findDepthFormat(physicalDevice);

  renderPass = Excal::Pipeline::createRenderPass(
    device,
    depthFormat,
    swapchainImageFormat,
    msaaSamples
  );

  descriptorSetLayout = Excal::Descriptor::createDescriptorSetLayout(device);

  // TODO Fill in the create info
  pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

  pipelineLayout = device.createPipelineLayout(
    vk::PipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 0, nullptr)
  );

  graphicsPipeline = Excal::Pipeline::createGraphicsPipeline(
    device,     pipelineLayout,  pipelineCache,
    renderPass, swapchainExtent, msaaSamples
  );

  commandPool = device.createCommandPool(
    vk::CommandPoolCreateInfo({}, queueFamilyIndices.graphicsFamily.value())
  );

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

  textureResources = Excal::Image::createTextureResources(
    physicalDevice, device,
    allocator,      commandPool,
    graphicsQueue,  config.modelDiffuseTexturePath
  );

  textureSampler = Excal::Image::createTextureImageSampler(
    device, textureResources.image
  );

  // Create vectors containing all model indices and vertices
  std::vector<uint32_t> indices;
  std::vector<Vertex>   vertices;

  for (auto& model : config.models) {
    // Make indices relative for each model
    for (auto& index : model.indices) {
      index += vertices.size();
    }
    indexCounts.push_back(model.indices.size());
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

  uniformBufferAllocations.resize(swapchainImageViews.size());

  uniformBuffers = Excal::Buffer::createUniformBuffers(
    physicalDevice, device,
    allocator,      uniformBufferAllocations
  );

  descriptorPool = Excal::Descriptor::createDescriptorPool(
    device, swapchainImages.size()
  );

  descriptorSets = Excal::Descriptor::createDescriptorSets(
    device,         swapchainImages.size(),
    descriptorPool, descriptorSetLayout,
    uniformBuffers, textureResources.imageView,
    textureSampler
  );

  commandBuffers = Excal::Buffer::createCommandBuffers(
    device,          commandPool,      swapchainFramebuffers,
    swapchainExtent, graphicsPipeline, pipelineLayout,
    indexCounts,     indexBuffer,      vertexBuffer,
    renderPass,      descriptorSets
  );
}

void Engine::mainLoop()
{
  size_t currentFrame = 0;

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    Excal::Frame::drawFrame(
      // Required for call to Excal::Swapchain::recreateSwpachain
      // but otherwise not for drawFrame()
      window,          descriptorPool,       commandBuffers,
      swapchain,       swapchainImageFormat, swapchainExtent,
      swapchainImages, swapchainImageViews,  swapchainFramebuffers,
      colorResources,  depthResources,       uniformBuffers,
      renderPass,      graphicsPipeline,     pipelineLayout,
      pipelineCache,   descriptorSets,       physicalDevice,
      surface,         msaaSamples,          depthFormat,
      commandPool,     indexCounts,          indexBuffer,
      vertexBuffer,    descriptorSetLayout,  textureSampler,
      textureResources.imageView, 

      // Required for regular drawFrame() functionality
      currentFrame,             framebufferResized,       allocator,
      uniformBufferAllocations, imagesInFlight,           device,
      graphicsQueue,            presentQueue,             inFlightFences,
      imageAvailableSemaphores, renderFinishedSemaphores, config.maxFramesInFlight
    );
  }

  device.waitIdle();
}

void Engine::cleanup()
{
  Excal::Swapchain::cleanupSwapchain(
    device,              allocator,             commandPool,
    descriptorPool,      commandBuffers,        swapchain,
    swapchainImageViews, swapchainFramebuffers, colorResources,
    depthResources,      uniformBuffers,        uniformBufferAllocations,
    renderPass,          graphicsPipeline,      pipelineCache,
    pipelineLayout
  );

  for (size_t i=0; i < config.maxFramesInFlight; i++) {
    device.destroySemaphore(imageAvailableSemaphores[i]);
    device.destroySemaphore(renderFinishedSemaphores[i]);
    device.destroyFence(inFlightFences[i]);
  }

  device.destroyDescriptorSetLayout(descriptorSetLayout);

  device.destroySampler(textureSampler);
  device.destroyImageView(textureResources.imageView);

  vmaDestroyBuffer(allocator, indexBuffer, indexBufferAllocation);
  vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);

  vmaDestroyImage(allocator, textureResources.image, textureResources.imageAllocation);

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
