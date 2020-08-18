#include <chrono>

#include "engine.h"
#include "structs.h"

#include "modelViewer.h"
#include "terrainGenerator.h"

int main()
{
  Excal::Engine excal;

  auto config = excal.createEngineConfig();

  App::ModelViewer::run(config);
  //App::TerrainGenerator::run(config);

  excal.init(config);
  excal.run();
}
