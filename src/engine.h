#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>

#include "image.h"
#include "model.h"

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
  vk::Buffer                     indexBuffer;
  vk::Buffer                     vertexBuffer;
  vk::DeviceMemory               indexBufferMemory;
  vk::DeviceMemory               vertexBufferMemory;
  vk::CommandPool                commandPool;
  std::vector<vk::CommandBuffer> commandBuffers;
  std::vector<vk::Buffer>        uniformBuffers;
  std::vector<vk::DeviceMemory>  uniformBuffersMemory;
  std::vector<VkFramebuffer>     swapchainFramebuffers;

  // Set by Excal::Model
  Excal::Model::ModelData modelData;

  // Application config
  int maxFramesInFlight = 3; // Triple buffering

  uint32_t windowWidth  = 1440;
  uint32_t windowHeight = 900;

  std::string modelPath          = "../models/ivysaur.obj";
  std::string diffuseTexturePath = "../textures/ivysaur_diffuse.jpg";

  //#define NDEBUG
  #ifdef NDEBUG
    const bool validationLayersEnabled = false;
  #else
    const bool validationLayersEnabled = true;
  #endif


  void init();
  void cleanup();
  void mainLoop();

public:
  Engine();
  ~Engine();
  int run();
};
}
