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

int main() {
  try {
    // Application config
    const uint32_t windowWidth  = 1440;
    const uint32_t windowHeight = 900;

    const int maxFramesInFlight = 2;

    const std::string modelPath          = "../models/ivysaur.obj";
    const std::string diffuseTexturePath = "../textures/ivysaur_diffuse.jpg";
    
    //#define NDEBUG
    #ifdef NDEBUG
      const bool validationLayersEnabled = false;
    #else
      const bool validationLayersEnabled = true;
    #endif

    // Debugger setup
    const bool validationLayersSupported = Excal::Debug::checkValidationLayerSupport(); 
    const auto validationLayers          = Excal::Debug::getValidationLayers();
    const auto debugMessengerCreateInfo  = Excal::Debug::getDebugMessengerCreateInfo();

    auto window = Excal::Surface::initWindow(windowWidth, windowHeight);

    auto instance = Excal::Device::createInstance(
      validationLayersEnabled,
      validationLayersSupported,
      validationLayers,
      debugMessengerCreateInfo
    );

    auto debugMessenger = Excal::Debug::setupDebugMessenger(
      instance, validationLayersEnabled, debugMessengerCreateInfo
    );

    auto surface = Excal::Surface::createSurface(instance, window);

    auto physicalDevice     = Excal::Device::pickPhysicalDevice(instance, surface);
    auto queueFamilyIndices = Excal::Device::findQueueFamilies(physicalDevice, surface);

    auto device = Excal::Device::createLogicalDevice(physicalDevice, queueFamilyIndices);

    auto graphicsQueue = device.getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
    auto presentQueue  = device.getQueue(queueFamilyIndices.presentFamily.value(), 0);

    auto msaaSamples = Excal::Device::getMaxUsableSampleCount(physicalDevice);

    // Create swapchain and image views
    auto swapchainState = Excal::Swapchain::createSwapchain(
      physicalDevice, device,
      surface, window,
      queueFamilyIndices
    );

    auto swapchain            = swapchainState.swapchain;
    auto swapchainImageFormat = swapchainState.swapchainImageFormat;
    auto swapchainExtent      = swapchainState.swapchainExtent;
    auto swapchainImages      = device.getSwapchainImagesKHR(swapchain);

    auto swapchainImageViews = Excal::Image::createImageViews(
      device, swapchainImages, swapchainImageFormat
    );

    // Create sync objects for each frame in flight
    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence>     inFlightFences;
    std::vector<vk::Fence>     imagesInFlight(maxFramesInFlight);
    for (int i=0; i < maxFramesInFlight; i++)
    {
      imageAvailableSemaphores.push_back(device.createSemaphore({}, nullptr));
      renderFinishedSemaphores.push_back(device.createSemaphore({}, nullptr));
      inFlightFences.push_back(device.createFence(
        vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled), nullptr
      ));
    }

    auto depthFormat = Excal::Image::findDepthFormat(physicalDevice);

    auto renderPass = Excal::Pipeline::createRenderPass(
      device,
      depthFormat,
      swapchainImageFormat,
      msaaSamples
    );

    auto descriptorSetLayout = Excal::Descriptor::createDescriptorSetLayout(device);

    // TODO Fill in the create info
    auto pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

    auto pipelineLayout = device.createPipelineLayout(
      vk::PipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 0, nullptr)
    );

    auto graphicsPipeline = Excal::Pipeline::createGraphicsPipeline(
      device,
      pipelineLayout,
      pipelineCache,
      renderPass,
      swapchainExtent,
      msaaSamples
    );

    auto commandPool = device.createCommandPool(
      vk::CommandPoolCreateInfo({}, queueFamilyIndices.graphicsFamily.value())
    );

    auto colorResources = Excal::Image::createColorResources(
      physicalDevice,
      device,
      swapchainImageFormat,
      swapchainExtent,
      msaaSamples
    );

    auto depthResources = Excal::Image::createDepthResources(
      physicalDevice,
      device,
      depthFormat,
      swapchainImageFormat,
      swapchainExtent,
      msaaSamples
    );

    auto swapchainFramebuffers = Excal::Buffer::createFramebuffers(
      device,
      swapchainImageViews,
      colorResources.imageView,
      depthResources.imageView,
      renderPass,
      swapchainExtent
    );

    auto textureResources = Excal::Image::createTextureResources(
      physicalDevice,
      device,
      commandPool,
      graphicsQueue,
      diffuseTexturePath
    );

    auto textureSampler = Excal::Image::createTextureImageSampler(
      device,
      textureResources.image
    );

    auto modelData = Excal::Model::loadModel(modelPath);

    vk::DeviceMemory vertexBufferMemory;
    auto vertexBuffer = Excal::Buffer::createVkBuffer(
      vertexBufferMemory,
      physicalDevice,
      device,
      modelData.vertices,
      vk::BufferUsageFlagBits::eVertexBuffer,
      commandPool,
      graphicsQueue
    );

    vk::DeviceMemory indexBufferMemory;
    auto indexBuffer = Excal::Buffer::createVkBuffer(
      indexBufferMemory,
      physicalDevice,
      device,
      modelData.indices,
      vk::BufferUsageFlagBits::eIndexBuffer,
      commandPool,
      graphicsQueue
    );

    std::vector<vk::DeviceMemory>  uniformBuffersMemory;
    auto uniformBuffers = Excal::Buffer::createUniformBuffers(
      uniformBuffersMemory,
      physicalDevice,
      device,
      swapchainImageViews.size()
    );

    auto descriptorPool = Excal::Descriptor::createDescriptorPool(device, swapchainImages.size());
    auto descriptorSets = Excal::Descriptor::createDescriptorSets(
      device,
      swapchainImages.size(),
      descriptorPool,
      descriptorSetLayout,
      uniformBuffers,
      textureResources.imageView,
      textureSampler
    );

    auto commandBuffers = Excal::Buffer::createCommandBuffers(
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

    // Main loop
    size_t currentFrame = 0;
    bool framebufferResized = false;

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

    // Cleanup swapchain
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

    // Application cleanup
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
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
