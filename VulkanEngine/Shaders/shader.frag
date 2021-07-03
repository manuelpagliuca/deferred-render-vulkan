#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragCol;
layout(location = 2) in vec3 fragNrm;
layout(location = 3) in vec2 fragTex;

layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) out vec4 outPos; 	 // World Position	
layout(location = 1) out vec4 outColour; // colour of the fragment
layout(location = 2) out vec4 outNormal; // Normal	

void main()
{
	outPos	 	= vec4(fragWorldPos, 1.0);
	outColour 	= vec4(texture(texture_sampler, fragTex).xyz, 1.0);
	outNormal 	= vec4(fragNrm, 1.0);
}
