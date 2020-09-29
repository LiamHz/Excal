#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UboView {
  mat4 view;
  mat4 proj;
  vec3 camPos;
  vec3 lightPos;
  vec3 lightColor;
} uboView;

layout(binding = 1) uniform UboInstance {
  mat4 model;
} uboInstance;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 camPos;
layout(location = 3) out vec3 lightPos;
layout(location = 4) out vec3 lightColor;

void main() {
  fragTexCoord = inTexCoord;
  fragPos      = uboInstance.model * vec4(inPosition.xyz, 1.0);
  camPos       = uboView.camPos;
  lightPos     = uboView.lightPos;
  lightColor   = uboView.lightColor;
  gl_Position  = uboView.proj * uboView.view * fragPos;
}
