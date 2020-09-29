#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "engine.h"

namespace Excal
{

// NOTE If member variables are uninitialized, their default value
//      is whatever value already happens to be in that memory
//      location. This leads to undefined behavior, where sometimes
//      your code will work as expected, and sometimes it will
//      mysteriously break.
Camera::Camera() : pos(glm::vec3(0, 0, 5)), yaw(0.0), pitch(0.0) {}

void Camera::updateView()
{
  // Constrain pitch values to 90 degrees
  pitch = std::fmin(pitch,  89.0f);
  pitch = std::fmax(pitch, -89.0f);

  // Set the direction vector of the camera by its yaw and pitch
  dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  dir.y = sin(glm::radians(pitch));
  dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

  // Use Gram-Schmidt process to get the camera's unit vectors
  dir   = glm::normalize(dir);
  right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), dir));
  up    = glm::normalize(glm::cross(dir, right));

  view = glm::lookAt(pos, pos + dir, up);
};

void Camera::handleInput(GLFWwindow* window, float deltaTime)
{
  // Keep camSpeed constant regardless of FPS
  float camSpeed = movementSpeed * deltaTime;

  // Change camera's position from WASD input
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    pos += dir * camSpeed;
  } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    pos -= dir * camSpeed;
  } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    pos -= right * camSpeed;
  } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    pos  += right * camSpeed;
  }
}

void Camera::mouseCallback(
  GLFWwindow* window,
  const double xPos,
  const double yPos
) {
  auto engine = static_cast<Excal::Engine*>(glfwGetWindowUserPointer(window));

  if (engine->firstMouse) {
    engine->lastX = xPos;
    engine->lastY = yPos;
    engine->firstMouse = false;
  }

  float xOffset = (xPos - engine->lastX) * engine->config.camera.mouseSensitivity;
  float yOffset = (yPos - engine->lastY) * engine->config.camera.mouseSensitivity;

  engine->lastX = xPos;
  engine->lastY = yPos;

  engine->config.camera.yaw   += xOffset;
  engine->config.camera.pitch -= yOffset;
}

glm::mat4 Camera::getView() const
{
  return view;
}
}
