#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace Excal
{
class Camera {
public:
  Camera();

  void updateView();
  glm::mat4 getView() const;
  void handleInput(GLFWwindow* window, float deltaTime);
  static void mouseCallback(
    GLFWwindow* window,
    const double xPos,
    const double yPos
  );

  glm::vec3 pos;
  float pitch;
  float yaw;

  float movementSpeed    = 40.0f;
  float mouseSensitivity = 0.05f;

private:
  glm::vec3 dir;
  glm::vec3 right;
  glm::vec3 up;
  glm::mat4 view;
};
}
