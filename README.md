# Excal

Excal is a real-time rendering engine built with Vulkan.

![gif of terrain generator](https://github.com/LiamHz/Excal/blob/master/docs/assets/terrainGenerator.gif "gif of terrain generator")

## Repo Structure
`app/` is where projects created with Excal are created. Example projects in this repo include a [Perlin noise procedural terrain generator](https://github.com/LiamHz/Excal/blob/master/app/terrainGenerator.cpp), and a [3D model viewer](https://github.com/LiamHz/Excal/blob/master/app/modelViewer.cpp).

`src/` contains the engine's source code, 3000+ lines of Vulkan. Hopefully you won't have to touch any of it while building a project with Excal, and can instead just set the engine configuration variables.

## Example Code
To build a project with Excal you need to load 3D models or create arrays of vertices and indices yourself, and set some engine configuration variables.

The code below loads in 3 obj models, and sets per-model and engine configuration info.

```cpp
#include "engine.h"

int main()
{
  // Create an instance of the Excal engine
  Excal::Engine excal;

  // Loading an obj model is also where its
  // diffuse texture, position, and scale are set
  auto model1 = Excal::Model::createModel(
    "../models/helmet.obj",
    "../textures/helmet_diffuse.png",
    glm::vec3(-2.0, 0.0, 0.0), 0.35
  );

  auto model2 = Excal::Model::createModel(
    "../models/ivysaur.obj",
    "../textures/ivysaur_diffuse.jpg",
    glm::vec3(0.0), 1.0
  );

  auto model3 = Excal::Model::createModel(
    "../models/helmet.obj",
    "../textures/helmet_diffuse.png",
    glm::vec3(2.0, 0.0, 0.0), 0.35
  );

  // Set additional per-model information
  model2.rotationsPerSecond = 0.125;
  model3.rotationsPerSecond = 0.25;

  // Set engine configuration info
  auto config = excal.createEngineConfig();

  config.windowWidth    = 1440*0.7;
  config.windowHeight   = 900 *0.7;
  config.models         = { model1, model2, model3 };
  config.vertShaderPath = "../shaders/shader.vert.spv";
  config.fragShaderPath = "../shaders/shader.frag.spv";

  // Initialize and run Excal
  excal.init(config);
  excal.run();
}
```

Here's the result of this code

![gif of model viewer](https://github.com/LiamHz/Excal/blob/master/docs/assets/modelViewer.gif "gif of model viewer")

## Build from Source
The following terminal command will clone this repository and build it from source.
```
git clone https://github.com/LiamHz/Excal.git && cd Excal && mkdir build && cd build && cmake .. && make
```
