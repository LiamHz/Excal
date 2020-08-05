#pragma once

#include <vulkan/vulkan.hpp>

namespace Excal::Descriptor
{
std::vector<vk::DescriptorSet> createDescriptorSets(
  const vk::Device&                 device,
  const int                         nDescriptorSets,
  const vk::DescriptorPool&         descriptorPool,
  const vk::DescriptorSetLayout&    descriptorSetLayout,
  const std::vector<vk::Buffer>&    uniformBuffers,
  const std::vector<vk::ImageView>& textureImageViews,
  const vk::Sampler&                textureSampler
);

vk::DescriptorSetLayout createDescriptorSetLayout(
  const vk::Device& device,
  const int nTextures
);

vk::DescriptorPool createDescriptorPool(
  const vk::Device& device,
  const int         nDescriptorSets,
  const int         nTextures
);
}
