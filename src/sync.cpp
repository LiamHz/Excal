#include "sync.h"

#include <vulkan/vulkan.hpp>
#include <vector>

namespace Excal::Sync
{
Excal::Sync::Semaphores createSemaphores(
  const vk::Device& device,
  const int         maxFramesInFlight
) {
  std::vector<vk::Semaphore> imageAvailableSemaphores(maxFramesInFlight);
  std::vector<vk::Semaphore> renderFinishedSemaphores(maxFramesInFlight);

  for (size_t i=0; i < maxFramesInFlight; i++) {
    imageAvailableSemaphores[i]
      = device.createSemaphore(vk::SemaphoreCreateInfo(), nullptr);

    renderFinishedSemaphores[i]
      = device.createSemaphore(vk::SemaphoreCreateInfo(), nullptr);
  }

  return Semaphores {imageAvailableSemaphores, renderFinishedSemaphores};
}

Excal::Sync::Fences createFences(
  const vk::Device& device,
  const int         maxFramesInFlight
) {
  std::vector<vk::Fence> inFlightFences(maxFramesInFlight);
  std::vector<vk::Fence> imagesInFlight(maxFramesInFlight);

  for (size_t i=0; i < maxFramesInFlight; i++) {
    inFlightFences[i]
      = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled), nullptr);
  }

  return Fences {inFlightFences, imagesInFlight};
}
}
