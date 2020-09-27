#pragma once

#include "vector"
#include "structs.h"

namespace Excal::Model
{
struct ModelData {
  std::vector<uint32_t> indices;
  std::vector<Vertex>   vertices;
};

struct Model {
  std::vector<uint32_t> indices;
  std::vector<Vertex>   vertices;
  std::string diffuseTexturePath = "../textures/ivysaur_diffuse.jpg";
  std::string normalTexturePath  = "../textures/ivysaur_normal.jpg";
  glm::vec3   position           = glm::vec3(0.0);
  float       scale              = 1.0;
  float       rotationsPerSecond = 0.0;
};

ModelData loadModel(const std::string& modelPath);

Model createModel(
  const std::string& modelPath,
  const glm::vec3    position,
  const float        scale,
  const std::string& diffuseTexturePath = "../textures/ivysaur_diffuse.jpg",
  const std::string& normalTexturePath  = "../textures/ivysaur_normal.jpg"
);
}
