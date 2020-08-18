#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "structs.h"
#include "engine.h"

namespace App::TerrainGenerator
{
void run(
  Excal::Engine::EngineConfig& config
);

glm::vec3 getColor(
  const int r,
  const int g,
  const int b
);

std::vector<float> generateNoiseMap(
  const int   xOffset,
  const int   yOffset,
  const int   chunkWidth,
  const int   chunkHeight,
  const int   octaves,
  const float noiseScale,
  const float persistence,
  const float lacunarity
);

std::vector<float> generateVertices(
  const std::vector<float>& noise_map,
  const float               waterHeight,
  const int                 xOffset,
  const int                 yOffset,
  const int                 chunkWidth,
  const int                 chunkHeight,
  const float               meshHeight
);

std::vector<uint32_t> generateIndices(
  const int chunkWidth,
  const int chunkHeight
);

std::vector<float> generateNormals(
  const std::vector<uint32_t>& indices,
  const std::vector<float>&    vertices
);

struct terrainColor {
  terrainColor(float _height, glm::vec3 _color) {
    height = _height;
    color = _color;
  };
  float height;
  glm::vec3 color;
};

std::vector<float> generateBiome(
  const std::vector<float>& vertices,
  const float               waterHeight,
  const int                 xOffset,
  const int                 yOffset,
  const float               meshHeight
);

Excal::Model::Model generateMapChunk(
  const int   xOffset,
  const int   yOffset,
  const int   xMapChunks,
  const int   yMapChunks,
  const int   chunkWidth,
  const int   chunkHeight,
  const float waterHeight,
  const float meshHeight,
  const int   octaves,
  const float noiseScale,
  const float persistence,
  const float lacunarity
);
}
