#include "modelViewer.h"

#include "model.h"
#include "engine.h"

namespace App::ModelViewer
{
void run(
  Excal::Engine::EngineConfig& config
) {
  auto model1 = Excal::Model::createModel(
    "../models/helmet.obj",
    glm::vec3(-2.0, 0.0, 0.0), 0.35,
    "../textures/helmet_diffuse.png"
  );

  auto model2 = Excal::Model::createModel(
    "../models/ivysaur.obj",
    glm::vec3(0.0), 1.0,
    "../textures/ivysaur_diffuse.jpg",
    "../textures/ivysaur_normal.jpg"
  );

  auto model3 = Excal::Model::createModel(
    "../models/helmet.obj",
    glm::vec3(2.0, 0.0, 0.0), 0.35,
    "../textures/helmet_diffuse.png"
  );

  model2.rotationsPerSecond = 0.125;
  model3.rotationsPerSecond = 0.25;

  config.windowWidth    = 1440*0.7;
  config.windowHeight   = 900 *0.7;
  config.models         = { model1, model2, model3 };
  config.vertShaderPath = "../shaders/shader.vert.spv";
  config.fragShaderPath = "../shaders/shader.frag.spv";
  config.camera.movementSpeed    = 3.0f;
  config.camera.mouseSensitivity = 0.03f;
}
}
