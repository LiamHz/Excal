#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace Excal::Sync
{
struct Semaphores {
  std::vector<vk::Semaphore> imageAvailableSemaphores;
  std::vector<vk::Semaphore> renderFinishedSemaphores;
};

struct Fences {
  std::vector<vk::Fence> inFlightFences;
  std::vector<vk::Fence> imagesInFlight;
};

Semaphores createSemaphores(const vk::Device&, const int maxFramesInFlight);
Fences     createFences(const vk::Device&, const int maxFramesInFlight);
}
