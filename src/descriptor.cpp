#include "descriptor.h"

#include <vulkan/vulkan.hpp>

#include "structs.h"

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
) {
  std::vector<vk::DescriptorSet>       descriptorSets(nDescriptorSets);
  std::vector<vk::DescriptorSetLayout> layouts(nDescriptorSets, descriptorSetLayout);

  descriptorSets = device.allocateDescriptorSets(
    vk::DescriptorSetAllocateInfo(
      descriptorPool, nDescriptorSets, layouts.data()
    )
  );

  for (size_t i=0; i < nDescriptorSets; i++) {
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

  return descriptorSets;
}

vk::DescriptorSetLayout createDescriptorSetLayout(
  const vk::Device& device
) {
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

 return device.createDescriptorSetLayout(
   vk::DescriptorSetLayoutCreateInfo({}, bindings.size(), bindings.data())
 );
}

vk::DescriptorPool createDescriptorPool(
  const vk::Device& device,
  const int         nDescriptorSets
) {
  vk::DescriptorPoolSize uniformBufferPoolSize(
    vk::DescriptorType::eUniformBuffer, nDescriptorSets
  );

  vk::DescriptorPoolSize samplerPoolSize(
    vk::DescriptorType::eCombinedImageSampler, nDescriptorSets
  );

  std::array<vk::DescriptorPoolSize, 2> poolSizes = {
    uniformBufferPoolSize,
    samplerPoolSize
  };

  return device.createDescriptorPool(
    vk::DescriptorPoolCreateInfo(
      {}, nDescriptorSets,
      poolSizes.size(), poolSizes.data()
    )
  );
}
}

