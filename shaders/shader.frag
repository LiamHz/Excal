#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler texSampler;
layout(binding = 3) uniform texture2D textures[32];

layout(push_constant) uniform PER_OBJECT {
	int imgIdx;
} pc;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
  // 2* accounts for the fact that there are two textures per model (diffuse and normal)
  outColor = texture(sampler2D(textures[2*pc.imgIdx + 1], texSampler), fragTexCoord);
}
