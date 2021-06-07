#version 450

/*// Array for triangle that fills screen
vec2 positions[3] = vec2[](
	vec2(3.0, -1.0),
	vec2(-1.0, -1.0),
	vec2(-1.0, 3.0)
);*/

layout(location = 0) out vec2 outUV;

void main()
{
	outUV 		= vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}