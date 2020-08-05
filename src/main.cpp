#include "engine.h"

int main()
{
  Excal::Engine excal;

  auto model1 = excal.createModel(
    "../models/ivysaur.obj",
    "../textures/ivysaur_diffuse.jpg",
    -1.3, 1.0
  );

  auto model2 = excal.createModel(
    "../models/helmet.obj",
    "../textures/helmet_diffuse.png",
    1.3, 0.35
  );

  auto config = excal.createEngineConfig();

  config.models = { model1, model2 };

  excal.init(config);
  excal.run();
}
