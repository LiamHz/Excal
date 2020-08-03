#include "engine.h"

int main()
{
  Excal::Engine excal;

  auto modelData = excal.loadModel("../models/ivysaur.obj");

  auto config = excal.createEngineConfig();

  config.modelData               = modelData;
  config.modelDiffuseTexturePath = "../textures/ivysaur_diffuse.jpg";

  excal.init(config);
  excal.run();
}
