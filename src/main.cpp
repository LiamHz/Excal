#include "engine.h"

int main()
{
  Excal::Engine excal;

  auto model1 = excal.createModel("../models/ivysaur.obj", "fooTexturePath", -1.2);
  auto model2 = excal.createModel("../models/ivysaur.obj", "fooTexturePath",  0.0);
  auto model3 = excal.createModel("../models/ivysaur.obj", "fooTexturePath",  1.2);

  auto config = excal.createEngineConfig();

  config.models                  = { model1, model2, model3};
  config.modelDiffuseTexturePath = "../textures/ivysaur_diffuse.jpg";

  excal.init(config);
  excal.run();
}
