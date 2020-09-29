#include "light.h"

#include <chrono>
#include <iostream>

namespace Excal::Light
{
Point::Point() :
  color(glm::vec3(1.0)),
  pos(glm::vec3(0.0)),
  startPos(glm::vec3(0.0)),
  endPos(glm::vec3(0.0)),
  patrolLength(3.0)
{}

Point::Point(glm::vec3 _pos) :
  color(glm::vec3(1.0)),
  pos(_pos),
  startPos(_pos),
  endPos(_pos),
  patrolLength(3.0)
{}

glm::vec3 Point::getPos()
{
  return pos;
}

void Point::setPos(const glm::vec3& _pos)
{
  pos = _pos;
  startPos = _pos;
  endPos = _pos;
}

void Point::patrol()
{
  if (startPos == endPos) {
    return;
  }

  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime      = std::chrono::high_resolution_clock::now();

  float time = std::chrono::duration<float, std::chrono::seconds::period>(
    currentTime - startTime
  ).count();

  // Gamma ranges from 0 to 1
  // Period is patrolLength seconds
  float gamma = (sin(3.14 * time / patrolLength) + 1.0) * 0.5;

  pos = startPos * gamma + endPos * (1.0f - gamma);
}
}
