#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vulkan/vulkan.hpp>

#include <chrono>
#include <fstream>
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
#include "sync.h"
#include "model.h"
#include "buffer.h"

const std::string TEXTURE_PATH = "../textures/ivysaur_diffuse.jpg";

class ExcalApplication {
public:
  void run() {
    window = Excal::Surface::initWindow(windowWidth, windowHeight);
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

  vk::Image                      textureImage;
  vk::DeviceMemory               textureImageMemory;
  vk::ImageView                  textureImageView;
  vk::Sampler                    textureSampler;

  vk::Image                      depthImage;
  vk::DeviceMemory               depthImageMemory;
  vk::ImageView                  depthImageView;

  // Render target
  vk::Image                      colorImage;
  vk::DeviceMemory               colorImageMemory;
  vk::ImageView                  colorImageView;
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

    physicalDevice = Excal::Device::pickPhysicalDevice(instance, surface);
    auto queueFamilyIndices = Excal::Device::findQueueFamilies(physicalDevice, surface);

    device = Excal::Device::createLogicalDevice(physicalDevice, queueFamilyIndices);

    graphicsQueue = device.getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
    presentQueue  = device.getQueue(queueFamilyIndices.presentFamily.value(), 0);

    msaaSamples   = Excal::Device::getMaxUsableSampleCount(physicalDevice);

    // Create swapchain and image views
    auto swapchainState = Excal::Swapchain::createSwapchain(
      physicalDevice, device,
      surface, window,
      queueFamilyIndices
    );

    swapchain            = swapchainState.swapchain;
    swapchainImageFormat = swapchainState.swapchainImageFormat;
    swapchainExtent      = swapchainState.swapchainExtent;

    swapchainImages     = device.getSwapchainImagesKHR(swapchain);

    swapchainImageViews = Excal::Swapchain::createImageViews(
      device, swapchainImages, swapchainImageFormat
    );

    auto semaphores = Excal::Sync::createSemaphores(device, maxFramesInFlight);
    imageAvailableSemaphores = semaphores.imageAvailableSemaphores;
    renderFinishedSemaphores = semaphores.renderFinishedSemaphores;

    auto fences = Excal::Sync::createFences(device, maxFramesInFlight);
    inFlightFences = fences.inFlightFences;
    imagesInFlight = fences.imagesInFlight;

    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    commandPool = device.createCommandPool(
      vk::CommandPoolCreateInfo({}, queueFamilyIndices.graphicsFamily.value())
    );
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createTextureImage();
    createTextureImageView();
    createTextureImageSampler();

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

    createDescriptorPool();
    createDescriptorSets();
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
    device.destroyImageView(textureImageView);
    device.destroyImage(textureImage);
    device.freeMemory(textureImageMemory);

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
    device.destroyImageView(colorImageView);
    device.destroyImage(colorImage);
    device.freeMemory(colorImageMemory);

    device.destroyImageView(depthImageView);
    device.destroyImage(depthImage);
    device.freeMemory(depthImageMemory);

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
      physicalDevice, device,
      surface, window,
      queueFamilyIndices
    );
    Excal::Swapchain::createImageViews(device, swapchainImages, swapchainImageFormat);
    createRenderPass();
    createGraphicsPipeline();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createCommandBuffers(modelData.indices.size());
  }

  void createRenderPass() {
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
      {}, findDepthFormat(),
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

    renderPass = device.createRenderPass(
      vk::RenderPassCreateInfo(
        {}, attachments.size(), attachments.data(),
        1, &subpass, 1, &dependency
      )
    );
  }

  void createDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding uboLayoutBinding(
      0, vk::DescriptorType::eUniformBuffer, 1,
      vk::ShaderStageFlagBits::eVertex, nullptr
    );

    vk::DescriptorSetLayoutBinding samplerLayoutBinding(
      1, vk::DescriptorType::eCombinedImageSampler, 1,
      vk::ShaderStageFlagBits::eFragment, nullptr
    );

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
      uboLayoutBinding,
      samplerLayoutBinding
    };

   descriptorSetLayout = device.createDescriptorSetLayout(
     vk::DescriptorSetLayoutCreateInfo({}, bindings.size(), bindings.data())
   );
  }

  void createGraphicsPipeline() {
    auto vertShaderCode = readFile("../shaders/shader.vert.spv");
    auto fragShaderCode = readFile("../shaders/shader.frag.spv");

    auto vertShaderModule = createShaderModule(vertShaderCode);
    auto fragShaderModule = createShaderModule(fragShaderCode);

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
      (float) swapchainExtent.width, (float) swapchainExtent.height,
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

    vk::PipelineRasterizationStateCreateInfo rasterizer(
      {}, VK_FALSE, VK_FALSE,
      vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
      vk::FrontFace::eCounterClockwise, VK_FALSE
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

    pipelineLayout = device.createPipelineLayout(
      vk::PipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 0, nullptr)
    );

    pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

    // Combine above structs to create graphics pipeline
    graphicsPipeline = device.createGraphicsPipeline(
      pipelineCache,
      vk::GraphicsPipelineCreateInfo(
        {},               shaderStages.size(), shaderStages.data(),
        &vertexInputInfo, &inputAssembly,      nullptr,
        &viewportState,   &rasterizer,         &multisampling,
        &depthStencil,    &colorBlending,      nullptr,
        pipelineLayout,   renderPass,          0
      )
    ).value;
    // Calling `.value` is a workaround for a known issue
    // Should be resolved after pull request #678 gets merged
    // https://github.com/KhronosGroup/Vulkan-Hpp/pull/678

    device.destroyShaderModule(vertShaderModule);
    device.destroyShaderModule(fragShaderModule);
  }

  void createFramebuffers() {
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

  // Images can have different layouts that affect how the pixels are stored in memory
  // When performing operations on images, you want to transition the image
  // to a layout that is optimal for that operation's performance
  void transitionImageLayout(
    vk::Image image, vk::Format format,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout
  ) {
    auto cmd = Excal::Buffer::beginSingleTimeCommands(device, commandPool);

    vk::PipelineStageFlagBits srcPipelineStage;
    vk::PipelineStageFlagBits dstPipelineStage;

    vk::AccessFlagBits srcAccessMask;
    vk::AccessFlagBits dstAccessMask;

    // Specify source and destination pipeline barriers
    if (   oldLayout == vk::ImageLayout::eUndefined
        && newLayout == vk::ImageLayout::eTransferDstOptimal
    ) {
      srcAccessMask = static_cast<vk::AccessFlagBits>(0);
      dstAccessMask = vk::AccessFlagBits::eTransferWrite;

      srcPipelineStage = vk::PipelineStageFlagBits::eTopOfPipe;
      dstPipelineStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal
               && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal
    ) {
      srcAccessMask = vk::AccessFlagBits::eTransferWrite;
      dstAccessMask = vk::AccessFlagBits::eShaderRead;

      srcPipelineStage = vk::PipelineStageFlagBits::eTransfer;
      dstPipelineStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
      throw std::invalid_argument("unsupported layout transition!");
    }

    vk::ImageMemoryBarrier barrier(
      srcAccessMask, dstAccessMask,
      oldLayout, newLayout,
      VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
      image,
      vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );

    cmd.pipelineBarrier(
      srcPipelineStage, dstPipelineStage,
      vk::DependencyFlagBits::eByRegion,
      0, nullptr,
      0, nullptr,
      1, &barrier
    );

    Excal::Buffer::endSingleTimeCommands(device, cmd, commandPool, graphicsQueue);
  }

  void copyBufferToImage(
    vk::Buffer buffer,
    vk::Image image,
    uint32_t width, uint32_t height
  ) {
    auto cmd = Excal::Buffer::beginSingleTimeCommands(device, commandPool);

    // Specify which part of the buffer is to be copied to which part of the image
    vk::BufferImageCopy copyRegion(
      0, 0, 0,
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
      vk::Offset3D(0, 0, 0),
      vk::Extent3D(width, height, 1)
    );

    cmd.copyBufferToImage(
      buffer, image,
      vk::ImageLayout::eTransferDstOptimal,
      1, &copyRegion
    );

    Excal::Buffer::endSingleTimeCommands(device, cmd, commandPool, graphicsQueue);
  }

  void createImage(
    uint32_t width, uint32_t height,
    vk::SampleCountFlagBits numSamples,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlagBits properties,
    vk::Image& image, vk::DeviceMemory& imageMemory
  ) {
    image = device.createImage(
      vk::ImageCreateInfo(
        {}, vk::ImageType::e2D, format,
        vk::Extent3D(width, height, 1), 1, 1,
        numSamples,
        tiling, usage,
        vk::SharingMode::eExclusive
      )
    );

    auto memRequirements = device.getImageMemoryRequirements(image);

    imageMemory = device.allocateMemory(
      vk::MemoryAllocateInfo(
        memRequirements.size,
        Excal::Utils::findMemoryType(
          physicalDevice, memRequirements.memoryTypeBits, properties
        )
      )
    );

    device.bindImageMemory(image, imageMemory, 0);
  }

  void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(
      TEXTURE_PATH.c_str(),
      &texWidth, &texHeight, &texChannels,
      STBI_rgb_alpha
    );

    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
      throw std::runtime_error("failed to load texture image!");
    }

    // Staging buffer is on the CPU
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    Excal::Buffer::createBuffer(
      stagingBuffer,
      stagingBufferMemory,
      physicalDevice,
      device,
      imageSize,
      vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible
      | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void* data = device.mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, (size_t) imageSize);
    device.unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(
      texWidth, texHeight,
      vk::SampleCountFlagBits::e1,  // TODO
      vk::Format::eR8G8B8A8Srgb,
      vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst
      | vk::ImageUsageFlagBits::eSampled,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      textureImage, textureImageMemory
    );

    transitionImageLayout(
      textureImage, vk::Format::eR8G8B8A8Srgb,
      vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal
    );

    copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);

    transitionImageLayout(
      textureImage, vk::Format::eR8G8B8A8Srgb,
      vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal
    );

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
  }

  void createTextureImageView() {
    textureImageView = Excal::Utils::createImageView(
      device, textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor
    );
  }

  void createTextureImageSampler() {
    textureSampler = device.createSampler(
      vk::SamplerCreateInfo(
        // Specify how to interpolate texels that are
        // magnified (oversampling) or minified (undersampling)
        {}, vk::Filter::eLinear, vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        // Sampler address mode specifies how to
        // read texels that are outside of the image
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        0.0f,
        VK_TRUE, 16.0f, // Anisotropic filtering
        VK_FALSE, vk::CompareOp::eAlways,
        0.0f, 0.0f
      )
    );
  }

  void createDescriptorPool() {
    vk::DescriptorPoolSize uniformBufferPoolSize(
      vk::DescriptorType::eUniformBuffer, swapchainImages.size()
    );

    vk::DescriptorPoolSize samplerPoolSize(
      vk::DescriptorType::eCombinedImageSampler, swapchainImages.size()
    );

    std::array<vk::DescriptorPoolSize, 2> poolSizes = {
      uniformBufferPoolSize,
      samplerPoolSize
    };

    descriptorPool = device.createDescriptorPool(
      vk::DescriptorPoolCreateInfo(
        {}, swapchainImages.size(),
        poolSizes.size(), poolSizes.data()
      )
    );
  }

  void createDescriptorSets() {
    descriptorSets.resize(swapchainImages.size());
    std::vector<vk::DescriptorSetLayout> layouts(
      swapchainImages.size(), descriptorSetLayout
    );

    descriptorSets = device.allocateDescriptorSets(
      vk::DescriptorSetAllocateInfo(
        descriptorPool, swapchainImages.size(), layouts.data()
      )
    );

    for (size_t i=0; i < swapchainImages.size(); i++) {
      vk::DescriptorBufferInfo bufferInfo(
        uniformBuffers[i], 0, sizeof(UniformBufferObject)
      );

      vk::DescriptorImageInfo imageInfo(
        textureSampler, textureImageView,
        vk::ImageLayout::eShaderReadOnlyOptimal
      );

      vk::WriteDescriptorSet uniformBufferDescriptorWrite(
        descriptorSets[i], 0, 0, 1,
        vk::DescriptorType::eUniformBuffer,
        nullptr, &bufferInfo, nullptr
      );

      vk::WriteDescriptorSet samplerDescriptorWrite(
        descriptorSets[i], 1, 0, 1,
        vk::DescriptorType::eCombinedImageSampler,
        &imageInfo, nullptr, nullptr
      );

      std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
        uniformBufferDescriptorWrite,
        samplerDescriptorWrite
      };

      device.updateDescriptorSets(
        descriptorWrites.size(),
        descriptorWrites.data(),
        0, nullptr
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

  void createColorResources() {
    vk::Format colorFormat = swapchainImageFormat;

    createImage(
      swapchainExtent.width, swapchainExtent.height,
      msaaSamples,
      colorFormat,
      vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment
      | vk::ImageUsageFlagBits::eColorAttachment,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      colorImage, colorImageMemory
    );

    colorImageView = Excal::Utils::createImageView(
      device, colorImage, colorFormat, vk::ImageAspectFlagBits::eColor
    );
  }

  void createDepthResources() {
    auto depthFormat = findDepthFormat();

    createImage(
      swapchainExtent.width, swapchainExtent.height,
      msaaSamples,
      depthFormat,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      depthImage, depthImageMemory
    );

    depthImageView = Excal::Utils::createImageView(
      device, depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth
    );
  }

  bool hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint
        || format == vk::Format::eD24UnormS8Uint;
  }

  vk::Format findSupportedFormat(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling, vk::FormatFeatureFlags features
  ) {
    for (auto format : candidates) {
      auto props = physicalDevice.getFormatProperties(format);

      if (   tiling == vk::ImageTiling::eLinear
          && (props.linearTilingFeatures & features) == features
      ) {
        return format;
      } else if (   tiling == vk::ImageTiling::eOptimal
                 && (props.optimalTilingFeatures & features) == features
      ) {
        return format;
      }
    }

    throw std::runtime_error("failed to find supported format!");
  }

  vk::Format findDepthFormat() {
    return findSupportedFormat(
      {
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint
      }, vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
  }

  static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
  }

  vk::ShaderModule createShaderModule(const std::vector<char>& code) {
    return device.createShaderModule(
      vk::ShaderModuleCreateInfo(
        {}, code.size(), reinterpret_cast<const uint32_t*>(code.data())
      )
    );
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
