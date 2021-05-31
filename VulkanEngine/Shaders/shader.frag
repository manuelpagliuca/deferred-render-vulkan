#version 450

layout(location = 0) in vec3 fragCol;
layout(location = 1) in vec2 fragTex;

layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

layout(set = 2, binding = 0) uniform UboLight {
	vec3 color;
	float ambient_intensity;
} ubo_light;

layout(location = 0) out vec4 outColour; 	// Final output colour (must also have location

void main()
{
	outColour = ubo_light.ambient_intensity * texture(texture_sampler, fragTex);
}