#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.hpp>

#include <chrono>
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

    createFramebuffers(colorResources.imageView, depthResources.imageView);

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

    Excal::Model::loadModel(modelData,  "../models/ivysaur.obj");

    Excal::Buffer::createVkBuffer(
      vertexBuffer,
      vertexBufferMemory,
      physicalDevice,
      device,
      modelData.vertices,
      vk::BufferUsageFlagBits::eVertexBuffer,
      commandPool,
      graphicsQueue
    );

    Excal::Buffer::createVkBuffer(
      indexBuffer,
      indexBufferMemory,
      physicalDevice,
      device,
      modelData.indices,
      vk::BufferUsageFlagBits::eIndexBuffer,
      commandPool,
      graphicsQueue
    );

    createUniformBuffers();

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

    createCommandBuffers(modelData.indices.size());
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      drawFrame();
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

  void recreateSwapChain() {
    // Handle widow minimization
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
    }

    device.waitIdle();

    // TODO Member variables (swapchain, swapchainImages etc) should be reassigned here
    auto queueFamilyIndices = Excal::Device::findQueueFamilies(physicalDevice, surface);
    Excal::Swapchain::createSwapchain(
      physicalDevice,
      device,
      surface,
      window,
      queueFamilyIndices
    );
    Excal::Image::createImageViews(device, swapchainImages, swapchainImageFormat);

    // Resource creation
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

    createFramebuffers(colorResources.imageView, depthResources.imageView);
    createUniformBuffers();
    descriptorPool = Excal::Descriptor::createDescriptorPool(device, swapchainImages.size());
    createCommandBuffers(modelData.indices.size());
  }

  void createFramebuffers(
    const vk::ImageView& colorImageView,
    const vk::ImageView& depthImageView
  ) {
    swapchainFramebuffers.resize(swapchainImageViews.size());

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
  }

  void createUniformBuffers() {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapchainImages.size());
    uniformBuffersMemory.resize(swapchainImages.size());

    for (size_t i=0; i < swapchainImages.size(); i++) {
      Excal::Buffer::createBuffer(
        uniformBuffers[i],
        uniformBuffersMemory[i],
        physicalDevice,
        device,
        bufferSize,
        vk::BufferUsageFlagBits::eUniformBuffer,
          vk::MemoryPropertyFlagBits::eHostVisible
        | vk::MemoryPropertyFlagBits::eHostCoherent
      );
    }
  }

  void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
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
      glm::vec3(0.0f, 1.0f, 3.0f), // Eye position
      glm::vec3(0.0f),            // Center position
      glm::vec3(0.0f, 1.0f, 0.0f) // Up Axis
    );

    ubo.proj = glm::perspective(
      glm::radians(45.0f),
      swapchainExtent.width / (float) swapchainExtent.height,
      0.1f, 10.0f
    );

    // Invert Y axis to acccount for difference between OpenGL and Vulkan
    ubo.proj[1][1] *= -1;

    void* data = device.mapMemory(uniformBuffersMemory[currentImage], 0, sizeof(ubo));
    memcpy(data, &ubo, sizeof(ubo));
    device.unmapMemory(uniformBuffersMemory[currentImage]);
  }

  void createCommandBuffers(uint32_t nIndices) {
    commandBuffers.resize(swapchainFramebuffers.size());

    commandBuffers = device.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(
        commandPool, vk::CommandBufferLevel::ePrimary, commandBuffers.size()
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

      vk::Buffer vertexBuffers[] = {vertexBuffer};
      vk::DeviceSize offsets[] = {0};

      cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
      cmd.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

      cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipelineLayout, 0, 1,
        &descriptorSets[i], 0, nullptr
      );

      cmd.drawIndexed(nIndices, 1, 0, 0, 0);

      cmd.endRenderPass();
      cmd.end();
    }
  }

  void drawFrame() {
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
      recreateSwapChain();
      return;
    }

    updateUniformBuffer(imageIndex);

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
      recreateSwapChain();
      return;
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
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
