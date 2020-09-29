#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace Excal::Light
{
class Point {
public:
  Point();
  Point(glm::vec3 _pos);

  glm::vec3 getPos();
  void setPos(const glm::vec3& _pos);
  void patrol();

  glm::vec3 color;
  glm::vec3 startPos;
  glm::vec3 endPos;
  float patrolLength;

private:
  glm::vec3 pos;
};
}
