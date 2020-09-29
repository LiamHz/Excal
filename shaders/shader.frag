#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler texSampler;
layout(binding = 3) uniform texture2D textures[32];

layout(push_constant) uniform PER_OBJECT {
  int imgIdx;
} pc;

layout(location = 0) in vec4 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 camPos;
layout(location = 3) in vec3 lightPos;
layout(location = 4) in vec3 lightColor;

layout(location = 0) out vec4 outColor;

struct Light {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

vec3 calculateLighting(vec3 Normal, vec3 FragPos) {
  // Strength of ambient and specular lighting
  float ambientStrength  = 0.1;
  float specularStrength = 0.5;

  // TESTING Set ambient light strength of non-normal mapped
  //         model to approximately the same overall brightness
  //         as the normal mapped model
  if (pc.imgIdx == 0) {
    ambientStrength = 0.02;
  }

  // General variables for Phong lighting
  vec3 norm     = normalize(Normal);
  vec3 lightDir = normalize(lightPos - FragPos);

  // Ambient lighting
  vec3 ambient = ambientStrength * lightColor;
  
  // Diffuse lighting
  float diff   = max(dot(lightDir, norm), 0.0);
  vec3 diffuse = diff * lightColor;

  // Specular lighting
  vec3 viewDir    = normalize(camPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, Normal);
  float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32);
  vec3 specular   = specularStrength * spec * lightColor;
  
  return (ambient + diffuse + specular);
}

void main() {
  // 2* accounts for the fact that there are
  // two textures per model (diffuse and normal)
  vec3 inDiffuse = vec3(texture(sampler2D(
    textures[2*pc.imgIdx], texSampler), fragTexCoord
  ));

  vec3 normal = vec3(texture(sampler2D(
    textures[2*pc.imgIdx + 1], texSampler), fragTexCoord
  ));

  // Map from 0 to 1 (RGB) to -1 to 1 (XYZ normal vectors)
  normal = normalize(2.0 * normal - 1.0);

  // TESTING Don't apply normal map to the first model
  if (pc.imgIdx == 0) {
    normal = vec3(0.0, 0.0, -1.0);
  }
  if (pc.imgIdx == 1) {
    normal.z *= -1.0; // TODO Correct normal with TBN Matrix
  }

  vec3 lighting = calculateLighting(normal, vec3(fragPos));

  outColor = vec4(inDiffuse * lighting, 1.0);

  // TESTING Visualize normals on third model
  if (pc.imgIdx == 2) {
    outColor = vec4(normal, 1.0);
  }
}
