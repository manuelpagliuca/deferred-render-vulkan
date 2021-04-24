#version 450 		// Use GLSL 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

layout(set = 0, binding = 0) uniform UboViewProjection {
	mat4 projection;
	mat4 view;
} uboViewProjection;

// NOT IN USE, LEFT FOR REFERENCE (UBO NOT EFFICIENT AS PUSH COSTANT)
//layout(set = 0, binding = 1) uniform UboModel {
//	mat4 model;
//} uboModel;

// You can have only one PushCostant per Shader
layout(push_constant) uniform PushModel{
	mat4 model;
} pushModel;

layout(location = 0) out vec3 fragCol;

void main() {
	gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(pos, 1.0);
	
	fragCol = col;
}