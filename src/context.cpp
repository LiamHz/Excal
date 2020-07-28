#include "context.h"
#include <iostream>

namespace Excal
{
Context::Context() {}

void Context::setDebugContext(const DebugContext& debugContext)
{
  debug = debugContext;
}

void Context::setSurfaceContext(const SurfaceContext& surfaceContext)
{
  surface = surfaceContext;
}
}
