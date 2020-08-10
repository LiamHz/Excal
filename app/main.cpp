#include "engine.h"

int main()
{
  Excal::Engine excal;

  auto model1 = excal.createModel(
    "../models/helmet.obj",
    "../textures/helmet_diffuse.png",
    glm::vec3(-2.0, 0.0, 0.0), 0.35
  );

  auto model2 = excal.createModel(
    "../models/ivysaur.obj",
    "../textures/ivysaur_diffuse.jpg",
    glm::vec3(0.0), 1.0
  );

  auto model3 = excal.createModel(
    "../models/helmet.obj",
    "../textures/helmet_diffuse.png",
    glm::vec3(2.0, 0.0, 0.0), 0.35
  );

  auto config = excal.createEngineConfig();

  config.models = { model1, model2, model3};
  config.vertShaderPath = "../shaders/shader.vert.spv";
  config.fragShaderPath = "../shaders/shader.frag.spv";

  excal.init(config);
  excal.run();
}
