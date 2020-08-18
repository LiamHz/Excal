#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <optional>
#include <vector>

// TODO Move structs inside of Excal::Structs namespace
struct UboDynamicData {
  glm::mat4 *model = nullptr;
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  vk::SurfaceCapabilitiesKHR        surfaceCapabilities;
  std::vector<vk::SurfaceFormatKHR> surfaceFormats;
  std::vector<vk::PresentModeKHR>   presentModes;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec2 texCoord;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return vk::VertexInputBindingDescription(
      0, sizeof(Vertex), vk::VertexInputRate::eVertex
    );
  }

  static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions;

    // Position
    attributeDescriptions[0] = vk::VertexInputAttributeDescription(
      0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)
    );

    // Color
    attributeDescriptions[1] = vk::VertexInputAttributeDescription(
      1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)
    );

    // Normal
    attributeDescriptions[2] = vk::VertexInputAttributeDescription(
      2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)
    );

    // Texture coordinates
    attributeDescriptions[3] = vk::VertexInputAttributeDescription(
      3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)
    );

    return attributeDescriptions;
  }

  // Operator to test for equality between vertces, used in hash function
  bool operator==(const Vertex& other) const {
    return pos == other.pos &&
           color == other.color &&
           normal == other.normal &&
           texCoord == other.texCoord;
  }
};

// Hash function for vertices
namespace std {
  template<> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
      return ((((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color)  << 1)) >> 1) ^
                (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
  };
}

// Account for Vulkan aligment requirements
struct UniformBufferObject {
  glm::mat4 view;
  glm::mat4 proj;
};

struct DynamicUniformBufferObject {
  glm::mat4 model;
};
