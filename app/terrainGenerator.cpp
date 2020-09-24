#include "terrainGenerator.h"

#include "perlin.h"
#include "structs.h"
#include "engine.h"

namespace App::TerrainGenerator
{
void run(
  Excal::Engine::EngineConfig& config
) {
  config.appName        = "vkTerrainGenerator";
  config.windowWidth    = 1440*0.7;
  config.windowHeight   = 900 *0.7;
  config.vertShaderPath = "../shaders/terrainShader.vert.spv";
  config.fragShaderPath = "../shaders/terrainShader.frag.spv";
  config.frontFace      = "clockwise";
  config.clearColor     = glm::vec4(0.53, 0.81, 0.92, 1.0);
  config.farClipPlane   = 512.0;
  config.camera.pos     = glm::vec3(192, 70, 320);

  // App params
  const int xMapChunks    = 3;
  const int yMapChunks    = 3;
  const int chunkWidth    = 128;
  const int chunkHeight   = 128;
  const float waterHeight = 0.1;
  const float meshHeight  = 32;  // Vertical scaling

  // Noise params
  int octaves       = 5;
  float noiseScale  = 64;  // Horizontal scaling
  float persistence = 0.5;
  float lacunarity  = 2;

  // Generate map chunks
  for (int yPos = 0; yPos < yMapChunks; yPos++) {
    for (int xPos = 0; xPos < xMapChunks; xPos++) {
      config.models.push_back(
        generateMapChunk(
          xPos,        yPos,
          xMapChunks,  yMapChunks,
          chunkWidth,  chunkHeight,
          waterHeight, meshHeight,
          octaves,     noiseScale,
          persistence, lacunarity
        )
      );
    }
  }
}

glm::vec3 getColor(
  const int r,
  const int g,
  const int b
) {
  return glm::vec3(r/255.0, g/255.0, b/255.0);
}

std::vector<float> generateNoiseMap(
  const int   xOffset,
  const int   yOffset,
  const int   chunkWidth,
  const int   chunkHeight,
  const int   octaves,
  const float noiseScale,
  const float persistence,
  const float lacunarity
) {
  std::vector<float> noiseValues;
  std::vector<float> normalizedNoiseValues;
  std::vector<int> p = Perlin::get_permutation_vector();

  float amp  = 1;
  float freq = 1;
  float maxPossibleHeight = 0;

  for (int i = 0; i < octaves; i++) {
    maxPossibleHeight += amp;
    amp *= persistence;
  }

  for (int y = 0; y < chunkHeight; y++) {
    for (int x = 0; x < chunkWidth; x++) {
      amp  = 1;
      freq = 1;
      float noiseHeight = 0;
      for (int i = 0; i < octaves; i++) {
        float xSample = (x + xOffset * (chunkWidth-1))  / noiseScale * freq;
        float ySample = (y + yOffset * (chunkHeight-1)) / noiseScale * freq;

        float perlinValue = Perlin::perlin_noise(xSample, ySample, p);
        noiseHeight += perlinValue * amp;

        // Lacunarity  --> Increase in frequency of octaves
        // Persistence --> Decrease in amplitude of octaves
        amp  *= persistence;
        freq *= lacunarity;
      }

      noiseValues.push_back(noiseHeight);
    }
  }

  for (int y = 0; y < chunkHeight; y++) {
    for (int x = 0; x < chunkWidth; x++) {
      // Inverse lerp and scale values to range from 0 to 1
      normalizedNoiseValues.push_back(
        (noiseValues[x + y*chunkWidth] + 1) / maxPossibleHeight
      );
    }
  }

  return normalizedNoiseValues;
}

std::vector<float> generateVertices(
  const std::vector<float>& noise_map,
  const float               waterHeight,
  const int                 xOffset,
  const int                 yOffset,
  const int                 chunkWidth,
  const int                 chunkHeight,
  const float               meshHeight
) {
  std::vector<float> vertices;

  for (int z = 0; z < chunkHeight; z++) {
    for (int x = 0; x < chunkWidth; x++) {
      // Apply cubic easing to the noise
      float easedNoise = std::pow(noise_map[x + z*chunkWidth] * 1.1, 3);
      // Scale noise to match meshHeight
      // Pervent vertex height from being below waterHeight
      float y = std::fmax(easedNoise * meshHeight, waterHeight * 0.5 * meshHeight);
      vertices.push_back(x + xOffset * (chunkWidth - 1));
      vertices.push_back(y);
      vertices.push_back(z + yOffset * (chunkHeight - 1));
    }
  }

  return vertices;
}

std::vector<uint32_t> generateIndices(
  const int chunkWidth,
  const int chunkHeight
) {
  std::vector<uint32_t> indices;

  for (int y = 0; y < chunkHeight; y++) {
    for (int x = 0; x < chunkWidth; x++) {
      uint32_t pos = x + y*chunkWidth;

      if (x == chunkWidth - 1 || y == chunkHeight - 1) {
        // Don't create indices for right or top edge
        continue;
      } else {
        // Top left triangle of square
        indices.push_back(pos + chunkWidth);
        indices.push_back(pos);
        indices.push_back(pos + chunkWidth + 1);
        // Bottom right triangle of square
        indices.push_back(pos + 1);
        indices.push_back(pos + 1 + chunkWidth);
        indices.push_back(pos);
      }
    }
  }

  return indices;
}

std::vector<float> generateNormals(
  const std::vector<uint32_t>& indices,
  const std::vector<float>&    vertices
) {
  std::vector<float>     normals;
  std::vector<glm::vec3> verts;

  // Get the vertices of each triangle in mesh
  // For each group of indices
  for (uint32_t i = 0; i < indices.size(); i += 3) {
    // Get the vertices (point) for each index
    for (uint32_t j = 0; j < 3; j++) {
      uint32_t pos = indices[i+j]*3;
      verts.push_back(glm::vec3(vertices[pos], vertices[pos+1], vertices[pos+2]));
    }

    // Get vectors of two edges of triangle
    glm::vec3 U = verts[i+1] - verts[i];
    glm::vec3 V = verts[i+2] - verts[i];

    // Calculate normal
    glm::vec3 normal = glm::normalize(-glm::cross(U, V));
    normals.push_back(normal.x);
    normals.push_back(normal.y);
    normals.push_back(normal.z);
  }

  return normals;
}

std::vector<float> generateBiome(
  const std::vector<float> &vertices,
  const float              waterHeight,
  const int                xOffset,
  const int                yOffset,
  const float              meshHeight
) {
  std::vector<float> colors;
  std::vector<terrainColor> biomeColors;
  glm::vec3 color = getColor(255, 255, 255);

  // NOTE: Terrain color height is a value between 0 and 1
  auto wh = waterHeight;
  biomeColors.push_back(terrainColor(wh * 0.5, getColor(60,  95, 190))); // Deep water
  biomeColors.push_back(terrainColor(wh,       getColor(60, 100, 190))); // Shallow water
  biomeColors.push_back(terrainColor(0.15, getColor(210, 215, 130)));    // Sand
  biomeColors.push_back(terrainColor(0.30, getColor( 95, 165,  30)));    // Grass 1
  biomeColors.push_back(terrainColor(0.40, getColor( 65, 115,  20)));    // Grass 2
  biomeColors.push_back(terrainColor(0.50, getColor( 90,  65,  60)));    // Rock 1
  biomeColors.push_back(terrainColor(0.80, getColor( 75,  60,  55)));    // Rock 2
  biomeColors.push_back(terrainColor(1.00, getColor(255, 255, 255)));    // Snow

  // Determine which color to assign each vertex by its y-coord
  // Iterate through vertex y values
  for (int i = 1; i < vertices.size(); i += 3) {
    for (int j = 0; j < biomeColors.size(); j++) {
      // NOTE: The max height of a vertex is "meshHeight"
      if (vertices[i] <= biomeColors[j].height * meshHeight) {
        color = biomeColors[j].color;
        break;
      }
    }
    colors.push_back(color.r);
    colors.push_back(color.g);
    colors.push_back(color.b);
  }

  return colors;
}

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
) {
  // Generate map chunk data
  auto noiseMap = generateNoiseMap(
    xOffset,     yOffset,
    chunkWidth,  chunkHeight,
    octaves,     noiseScale,
    persistence, lacunarity
  );
  auto indices   = generateIndices(chunkWidth, chunkHeight);
  auto positions = generateVertices(noiseMap, waterHeight, xOffset, yOffset, chunkWidth, chunkHeight, meshHeight);
  auto normals   = generateNormals(indices, positions);
  auto colors    = generateBiome(positions, waterHeight, xOffset, yOffset, meshHeight);

  // Assemble vertices
  std::vector<Vertex> vertices;
  
  for (int i=0; i < positions.size() / 3; i++) {
    Vertex vertex = {
      glm::vec3(positions[i*3 + 0], positions[i*3 + 1], positions[i*3 + 2]),
      glm::vec3(colors[i*3 + 0],    colors[i*3 + 1],    colors[i*3 + 2]),
      glm::vec3(normals[i*3 + 0],   normals[i*3 + 1],   normals[i*3 + 2]),
      glm::vec3(0) // TexCoord isn't used
    };

    vertices.push_back(vertex);
  }

  Excal::Model::Model mapChunk;

  mapChunk.indices  = indices;
  mapChunk.vertices = vertices;
  mapChunk.position = glm::vec3(0.0);

  return mapChunk;
}
}
