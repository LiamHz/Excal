#pragma once

#include "vector"
#include "structs.h"

namespace Excal::Model
{
struct ModelData {
  std::vector<uint32_t> indices;
  std::vector<Vertex>   vertices;
};

ModelData loadModel(const std::string& modelPath);
}
