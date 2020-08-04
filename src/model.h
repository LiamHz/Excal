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
  std::string texturePath;
  MvpMatrix mvpMatrix;
};

ModelData loadModel(const std::string& modelPath);

Model createModel(
  const std::string& modelPath,
  const std::string& texturePath,
  const float        vertexOffset,  // TODO temp
  const MvpMatrix&   mvpMatrix
);
}
