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
  // TODO Removes the requirement for a model to have a texturePath
  std::string texturePath        = "../textures/ivysaur_diffuse.jpg";
  glm::vec3   position           = glm::vec3(0.0);
  float       scale              = 1.0;
  float       rotationsPerSecond = 0.0;
};

ModelData loadModel(const std::string& modelPath);

Model createModel(
  const std::string& modelPath,
  const std::string& texturePath,
  const glm::vec3    position,
  const float        scale
);
}
