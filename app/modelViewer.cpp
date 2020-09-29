#include "modelViewer.h"

#include "model.h"
#include "engine.h"

namespace App::ModelViewer
{
void run(
  Excal::Engine::EngineConfig& config
) {
  auto model1 = Excal::Model::createModel(
    "../models/wall.obj",
    glm::vec3(-1.0, 0.0, 0.0), 1.0,
    "../textures/wall_diffuse.jpg",
    "../textures/wall_normal.jpg"
  );

  auto model2 = Excal::Model::createModel(
    "../models/wall.obj",
    glm::vec3(0.0, 0.0, 0.0), 1.0,
    "../textures/wall_diffuse.jpg",
    "../textures/wall_normal.jpg"
  );

  auto model3 = Excal::Model::createModel(
    "../models/wall.obj",
    glm::vec3(1.0, 0.0, 0.0), 1.0,
    "../textures/wall_diffuse.jpg",
    "../textures/wall_normal.jpg"
  );

  config.windowWidth             = 1440*0.9;
  config.windowHeight            = 900 *0.9;
  config.models                  = { model1, model2, model3 };
  config.vertShaderPath          = "../shaders/shader.vert.spv";
  config.fragShaderPath          = "../shaders/shader.frag.spv";
  config.camera.movementSpeed    = 1.5f;
  config.camera.mouseSensitivity = 0.025f;

  config.light.setPos(glm::vec3(-2.0f, 1.0f, -2.0f));
  config.light.endPos = glm::vec3(2.0f, 1.0f, -2.0f);
  //config.light.color  = glm::vec3(1.0, 0.2, 0.1);
}
}
