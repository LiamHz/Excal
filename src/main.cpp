#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <array>

#include "structs.h"
#include "utils.h"
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

class ExcalApplication {
public:
  void run() {
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  VkDebugUtilsMessengerEXT       debugMessenger;

  vk::PipelineLayout             pipelineLayout;
  vk::PipelineCache              pipelineCache;
  vk::Pipeline                   graphicsPipeline;

  vk::DescriptorPool             descriptorPool;
  vk::DescriptorSetLayout        descriptorSetLayout;
  std::vector<vk::DescriptorSet> descriptorSets;

  vk::RenderPass                 renderPass;
  vk::CommandPool                commandPool;
  std::vector<vk::CommandBuffer> commandBuffers;

  std::vector<vk::Buffer>        uniformBuffers;
  std::vector<vk::DeviceMemory>  uniformBuffersMemory;

  std::vector<VkFramebuffer>     swapchainFramebuffers;

  size_t                         currentFrame = 0;
  bool                           framebufferResized = false;

  const uint32_t                 windowWidth  = 1440;
  const uint32_t                 windowHeight = 900;

  const int maxFramesInFlight = 2;

  // Set by Excal::Surface
  GLFWwindow*    window;
  vk::SurfaceKHR surface;

  // TODO Remove these as class members
  // (Requires changing dependent functions in main.cpp)
  // Set by Excal::Device
  vk::Instance            instance;
  vk::PhysicalDevice      physicalDevice;
  vk::Device              device;
  vk::Queue               graphicsQueue;
  vk::Queue               presentQueue;
  vk::SampleCountFlagBits msaaSamples;

  // Set by Excal::Swapchain
  vk::SwapchainKHR           swapchain;
  vk::Format                 swapchainImageFormat;
  vk::Extent2D               swapchainExtent;
  std::vector<vk::Image>     swapchainImages;
  std::vector<vk::ImageView> swapchainImageViews;

  Excal::Model::ModelData modelData;

  // Set by Excal::Sync
  std::vector<vk::Semaphore> imageAvailableSemaphores;
  std::vector<vk::Semaphore> renderFinishedSemaphores;
  std::vector<vk::Fence>     inFlightFences;
  std::vector<vk::Fence>     imagesInFlight;

  // Set by Excal::Buffer
  vk::Buffer       indexBuffer;
  vk::Buffer       vertexBuffer;
  vk::DeviceMemory indexBufferMemory;
  vk::DeviceMemory vertexBufferMemory;

  // Set by Excal::Image
  vk::Sampler                  textureSampler;
  vk::Format                   depthFormat;
  Excal::Image::ImageResources textureResources;
  Excal::Image::ImageResources colorResources;
  Excal::Image::ImageResources depthResources;

  //#define NDEBUG
  #ifdef NDEBUG
    const bool validationLayersEnabled = false;
  #else
    const bool validationLayersEnabled = true;
  #endif

  void initVulkan() {
    const bool validationLayersSupported = Excal::Debug::checkValidationLayerSupport(); 
    const auto validationLayers          = Excal::Debug::getValidationLayers();
    const auto debugMessengerCreateInfo  = Excal::Debug::getDebugMessengerCreateInfo();

    window = Excal::Surface::initWindow(windowWidth, windowHeight);

    instance = Excal::Device::createInstance(
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
    imagesInFlight.resize(maxFramesInFlight);
    for (int i=0; i < maxFramesInFlight; i++)
    {
      imageAvailableSemaphores.push_back(device.createSemaphore({}, nullptr));
      renderFinishedSemaphores.push_back(device.createSemaphore({}, nullptr));
      inFlightFences.push_back(device.createFence(
        vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled), nullptr
      ));
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
      device,
      pipelineLayout,
      pipelineCache,
      renderPass,
      swapchainExtent,
      msaaSamples
    );

    commandPool = device.createCommandPool(
      vk::CommandPoolCreateInfo({}, queueFamilyIndices.graphicsFamily.value())
    );

    Excal::Image::createColorResources(
      colorResources,
      physicalDevice,
      device,
      swapchainImageFormat,
      swapchainExtent,
      msaaSamples
    );

    Excal::Image::createDepthResources(
      depthResources,
      physicalDevice,
      device,
      depthFormat,
      swapchainImageFormat,
      swapchainExtent,
      msaaSamples
    );

    swapchainFramebuffers = Excal::Buffer::createFramebuffers(
      device,
      swapchainImageViews,
      colorResources.imageView,
      depthResources.imageView,
      renderPass,
      swapchainExtent
    );

    const std::string texturePath = "../textures/ivysaur_diffuse.jpg";
    Excal::Image::createTextureResources(
      textureResources,
      physicalDevice,
      device,
      commandPool,
      graphicsQueue,
      texturePath
    );

    textureSampler = Excal::Image::createTextureImageSampler(
      device,
      textureResources.image
    );

    Excal::Model::loadModel(modelData, "../models/ivysaur.obj");

    vertexBuffer = Excal::Buffer::createVkBuffer(
      vertexBufferMemory,
      physicalDevice,
      device,
      modelData.vertices,
      vk::BufferUsageFlagBits::eVertexBuffer,
      commandPool,
      graphicsQueue
    );

    indexBuffer = Excal::Buffer::createVkBuffer(
      indexBufferMemory,
      physicalDevice,
      device,
      modelData.indices,
      vk::BufferUsageFlagBits::eIndexBuffer,
      commandPool,
      graphicsQueue
    );

    uniformBuffers = Excal::Buffer::createUniformBuffers(
      uniformBuffersMemory,
      physicalDevice,
      device,
      swapchainImageViews.size()
    );

    descriptorPool = Excal::Descriptor::createDescriptorPool(device, swapchainImages.size());
    descriptorSets = Excal::Descriptor::createDescriptorSets(
      device,
      swapchainImages.size(),
      descriptorPool,
      descriptorSetLayout,
      uniformBuffers,
      textureResources.imageView,
      textureSampler
    );

    commandBuffers = Excal::Buffer::createCommandBuffers(
      device,
      commandPool,
      swapchainFramebuffers,
      swapchainExtent,
      modelData.indices.size(),
      graphicsPipeline,
      vertexBuffer,
      indexBuffer,
      renderPass,
      descriptorSets,
      pipelineLayout
    );
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      Excal::Frame::drawFrame(
        // Required for call to Excal::Swapchain::recreateSwpachain
        // but otherwise not for drawFrame()
        window,          descriptorPool,       commandBuffers,
        swapchain,       swapchainImageFormat, swapchainExtent,
        swapchainImages, swapchainImageViews,  swapchainFramebuffers,
        colorResources,  depthResources,       uniformBuffers,
        physicalDevice,  surface,              msaaSamples,
        depthFormat,     renderPass,           modelData.indices.size(),
        commandPool,     graphicsPipeline,     vertexBuffer,
        indexBuffer,     pipelineLayout,       descriptorSets,

        // Required for regular drawFrame() functionality
        currentFrame,       uniformBuffersMemory,     imagesInFlight,
        device,             graphicsQueue,            presentQueue,
        inFlightFences,     imageAvailableSemaphores, renderFinishedSemaphores,
        framebufferResized, maxFramesInFlight
      );
    }

    device.waitIdle();
  }

  void cleanup() {
    cleanupSwapChain();

    for (size_t i=0; i < maxFramesInFlight; i++) {
      device.destroySemaphore(imageAvailableSemaphores[i]);
      device.destroySemaphore(renderFinishedSemaphores[i]);
      device.destroyFence(inFlightFences[i]);
    }

    device.destroyDescriptorSetLayout(descriptorSetLayout);

    device.destroyBuffer(indexBuffer);
    device.freeMemory(indexBufferMemory);

    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);

    device.destroySampler(textureSampler);
    device.destroyImageView(textureResources.imageView);
    device.destroyImage(textureResources.image);
    device.freeMemory(textureResources.imageMemory);

    device.destroyCommandPool(commandPool);
    device.destroy();

    if (validationLayersEnabled) {
      Excal::Debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    instance.destroySurfaceKHR(surface);
    instance.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void cleanupSwapChain() {
    device.destroyImageView(colorResources.imageView);
    device.destroyImage(colorResources.image);
    device.freeMemory(colorResources.imageMemory);

    device.destroyImageView(depthResources.imageView);
    device.destroyImage(depthResources.image);
    device.freeMemory(depthResources.imageMemory);

    for (auto framebuffer : swapchainFramebuffers) {
      device.destroyFramebuffer(framebuffer);
    }

    device.freeCommandBuffers(commandPool, commandBuffers);

    device.destroyDescriptorPool(descriptorPool);

    device.destroyPipeline(graphicsPipeline);
    device.destroyPipelineCache(pipelineCache);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyRenderPass(renderPass);

    for (auto imageView : swapchainImageViews) {
      device.destroyImageView(imageView);
    }

    for (size_t i=0; i < swapchainImages.size(); i++) {
      device.destroyBuffer(uniformBuffers[i]);
      device.freeMemory(uniformBuffersMemory[i]);
    }

    device.destroySwapchainKHR(swapchain);
  }
};

int main() {
  ExcalApplication app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
