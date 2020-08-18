#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform UboView {
  mat4 view;
  mat4 proj;
} uboView;

layout (binding = 1) uniform UboInstance {
  mat4 model;
} uboInstance;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec3 fragColor;

struct Light {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 direction;
};

vec3 calculateLighting(vec3 Normal, vec3 FragPos) {
  // TODO Define this in engine config
  Light light;
  light.ambient   = vec3(0.2, 0.2, 0.2);
  light.diffuse   = vec3(0.5, 0.5, 0.5);
  light.specular  = vec3(1.0, 1.0, 1.0);
  light.direction = vec3(-0.2f, -1.0f, -0.3);

  // Ambient lighting
  vec3 ambient = light.ambient;
  
  // Diffuse lighting
  vec3 norm     = normalize(Normal);
  vec3 lightDir = normalize(-light.direction);
  float diff    = max(dot(lightDir, norm), 0.0);
  vec3 diffuse  = light.diffuse * diff;

  // Specular lighting
  //float specularStrength = 0.5;
  //vec3 viewDir = normalize(u_viewPos - FragPos);
  //vec3 reflectDir = reflect(-lightDir, Normal);

  //float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
  //vec3 specular = light.specular * spec;
  
  //return (ambient + diffuse + specular);
  return (ambient + diffuse);
}

void main() {
  vec4 fragPos = uboInstance.model * vec4(inPosition.xyz, 1.0);
  gl_Position  = uboView.proj * uboView.view * fragPos;

  // TODO Normals are 'streaky' try inverting them
  //vec3 transformedNormal = transpose(inverse(mat3(uboInstance.model))) * inNormal;
  //vec3 lighting = calculateLighting(transformedNormal, vec3(fragPos));

  vec3 lighting = calculateLighting(inNormal, vec3(fragPos));

  fragColor = inColor * lighting;
}
