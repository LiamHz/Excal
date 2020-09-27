#include "descriptor.h"

#include <vulkan/vulkan.hpp>

#include "structs.h"

namespace Excal::Descriptor
{
std::vector<vk::DescriptorSet> createDescriptorSets(
  const vk::Device&                 device,
  const int                         nDescriptorSets,
  const vk::DescriptorPool&         descriptorPool,
  const vk::DescriptorSetLayout&    descriptorSetLayout,
  const std::vector<vk::Buffer>&    uniformBuffers,
  const std::vector<vk::Buffer>&    dynamicUniformBuffers,
  const std::vector<vk::ImageView>& textureImageViews,
  const vk::Sampler&                textureSampler
) {
  std::vector<vk::DescriptorSet>       descriptorSets(nDescriptorSets);
  std::vector<vk::DescriptorSetLayout> layouts(nDescriptorSets, descriptorSetLayout);

  descriptorSets = device.allocateDescriptorSets(
    vk::DescriptorSetAllocateInfo(
      descriptorPool, nDescriptorSets, layouts.data()
    )
  );

  for (size_t i=0; i < nDescriptorSets; i++) {
    vk::DescriptorBufferInfo uniformBufferInfo(
      uniformBuffers[i], 0,
      sizeof(UniformBufferObject)
    );

    vk::DescriptorBufferInfo dynamicUniformBufferInfo(
      dynamicUniformBuffers[i], 0,
      sizeof(DynamicUniformBufferObject)
    );

    vk::DescriptorImageInfo textureImageInfos[textureImageViews.size()];

    for (int j=0; j < textureImageViews.size(); j++) {
      textureImageInfos[j].sampler     = nullptr;
      textureImageInfos[j].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      textureImageInfos[j].imageView   = textureImageViews[j];
    }

    vk::DescriptorImageInfo textureSamplerInfo(textureSampler);

    vk::WriteDescriptorSet uniformBufferDescriptorWrite(
      descriptorSets[i], 0, 0, 1,
      vk::DescriptorType::eUniformBuffer,
      nullptr, &uniformBufferInfo, nullptr
    );

    vk::WriteDescriptorSet dynamicUniformBufferDescriptorWrite(
      descriptorSets[i], 1, 0, 1,
      vk::DescriptorType::eUniformBufferDynamic,
      nullptr, &dynamicUniformBufferInfo, nullptr
    );

    vk::WriteDescriptorSet textureSamplerDescriptorWrite(
      descriptorSets[i], 2, 0, 1,
      vk::DescriptorType::eSampler,
      &textureSamplerInfo, nullptr, nullptr
    );

    vk::WriteDescriptorSet textureImageDescriptorWrite(
      descriptorSets[i], 3, 0, textureImageViews.size(),
      vk::DescriptorType::eSampledImage,
      textureImageInfos, nullptr, nullptr
    );

    std::array<vk::WriteDescriptorSet, 4> descriptorWrites = {
      uniformBufferDescriptorWrite,
      dynamicUniformBufferDescriptorWrite,
      textureSamplerDescriptorWrite,
      textureImageDescriptorWrite
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
  const vk::Device& device,
  const int nTextures
) {
  vk::DescriptorSetLayoutBinding uboLayoutBinding(
    0, vk::DescriptorType::eUniformBuffer,
    1, vk::ShaderStageFlagBits::eVertex, nullptr
  );

  vk::DescriptorSetLayoutBinding dynamicUboLayoutBinding(
    1, vk::DescriptorType::eUniformBufferDynamic,
    1, vk::ShaderStageFlagBits::eVertex, nullptr
  );

  vk::DescriptorSetLayoutBinding textureSamplerLayoutBinding(
    2, vk::DescriptorType::eSampler,
    1, vk::ShaderStageFlagBits::eFragment, nullptr
  );

  vk::DescriptorSetLayoutBinding textureImageLayoutBinding(
    3,         vk::DescriptorType::eSampledImage,
    nTextures, vk::ShaderStageFlagBits::eFragment, nullptr
  );

  std::array<vk::DescriptorSetLayoutBinding, 4> bindings = {
    uboLayoutBinding,
    dynamicUboLayoutBinding,
    textureSamplerLayoutBinding,
    textureImageLayoutBinding
  };

 return device.createDescriptorSetLayout(
   vk::DescriptorSetLayoutCreateInfo({}, bindings.size(), bindings.data())
 );
}

vk::DescriptorPool createDescriptorPool(
  const vk::Device& device,
  const int         nDescriptorSets,
  const int         nTextures
) {
  vk::DescriptorPoolSize uniformBufferPoolSize(
    vk::DescriptorType::eUniformBuffer, nDescriptorSets
  );

  vk::DescriptorPoolSize dynamicUniformBufferPoolSize(
    vk::DescriptorType::eUniformBufferDynamic, nDescriptorSets
  );

  vk::DescriptorPoolSize textureSamplerPoolSize(
    vk::DescriptorType::eSampler, nDescriptorSets
  );

  vk::DescriptorPoolSize textureImagePoolSize(
    vk::DescriptorType::eSampledImage, nDescriptorSets * nTextures
  );

  std::array<vk::DescriptorPoolSize, 4> poolSizes = {
    uniformBufferPoolSize,
    textureSamplerPoolSize,
    textureImagePoolSize,
    dynamicUniformBufferPoolSize
  };

  return device.createDescriptorPool(
    vk::DescriptorPoolCreateInfo(
      {}, nDescriptorSets,
      poolSizes.size(), poolSizes.data()
    )
  );
}
}

