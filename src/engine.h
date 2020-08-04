#pragma once

// Silence Clang warnings for VMA
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wnullability-completeness"

#include <vk_mem_alloc.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>

#include "image.h"
#include "model.h"
#include "structs.h"

namespace Excal
{
class Engine
{
private:
  // Set by Excal::Surface
  GLFWwindow*    window;
  vk::SurfaceKHR surface;
  bool framebufferResized = false;

  // Set by Excal::Debug
  VkDebugUtilsMessengerEXT debugMessenger;

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

  // Set by Excal::Sync
  std::vector<vk::Semaphore> imageAvailableSemaphores;
  std::vector<vk::Semaphore> renderFinishedSemaphores;
  std::vector<vk::Fence>     inFlightFences;
  std::vector<vk::Fence>     imagesInFlight;

  // Set by Excal::Image
  vk::Sampler                  textureSampler;
  vk::Format                   depthFormat;
  Excal::Image::ImageResources textureResources;
  Excal::Image::ImageResources colorResources;
  Excal::Image::ImageResources depthResources;

  // Set by Excal::Pipeline
  vk::PipelineLayout pipelineLayout;
  vk::PipelineCache  pipelineCache;
  vk::Pipeline       graphicsPipeline;
  vk::RenderPass     renderPass;

  // Set by Excal::Descriptor
  vk::DescriptorPool             descriptorPool;
  vk::DescriptorSetLayout        descriptorSetLayout;
  std::vector<vk::DescriptorSet> descriptorSets;

  // Set by Excal::Buffer
  std::vector<uint32_t>          indexCounts;
  vk::Buffer                     indexBuffer;
  vk::Buffer                     vertexBuffer;
  vk::CommandPool                commandPool;
  std::vector<vk::CommandBuffer> commandBuffers;
  std::vector<vk::Buffer>        uniformBuffers;
  std::vector<VkFramebuffer>     swapchainFramebuffers;

  // Set by Vulkan Memory Allocator
  VmaAllocator               allocator;
  VmaAllocation              indexBufferAllocation;
  VmaAllocation              vertexBufferAllocation;
  std::vector<VmaAllocation> uniformBufferAllocations;

  //#define NDEBUG
  #ifdef NDEBUG
    const bool validationLayersEnabled = false;
  #else
    const bool validationLayersEnabled = true;
  #endif

  struct EngineConfig {
    std::vector<Excal::Model::Model> models;
    std::string             modelDiffuseTexturePath;
    std::string appName   = "Excal Test App";
    int appVersion        = 1.0;
    uint32_t windowWidth  = 1440;
    uint32_t windowHeight = 900;
    int maxFramesInFlight = 3; // Triple buffering
  };

  EngineConfig config;

  void initVulkan();
  void cleanup();
  void mainLoop();

public:
  Engine();
  ~Engine();
  int run();

  EngineConfig createEngineConfig()
  {
    return EngineConfig{};
  }

  Excal::Model::Model createModel(
    const std::string& modelPath,
    const std::string& texturePath,
    const float        vertexOffset
    //const MvpMatrix&   mvpMatrix
  ) {
    return Excal::Model::createModel(
      modelPath, texturePath, vertexOffset,
      MvpMatrix { glm::mat4(1.0), glm::mat4(1.0), glm::mat4(1.0) }
    );
  }

  void init(const EngineConfig& _config)
  {
    config = _config;
    initVulkan();
  }
};
}
