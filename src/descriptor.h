#pragma once

#include <vulkan/vulkan.hpp>

namespace Excal::Descriptor
{
std::vector<vk::DescriptorSet> createDescriptorSets(
  const vk::Device&              device,
  const int                      nDescriptorSets,
  const vk::DescriptorPool&      descriptorPool,
  const vk::DescriptorSetLayout& descriptorSetLayout,
  const std::vector<vk::Buffer>& uniformBuffers,
  const vk::ImageView&           textureImageView,
  const vk::Sampler&             textureSampler
);

vk::DescriptorSetLayout createDescriptorSetLayout(
  const vk::Device& device
);

vk::DescriptorPool createDescriptorPool(
  const vk::Device& device,
  const int         nDescriptorSets
);
}
