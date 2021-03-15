#version 450

// Prelevo dal Vertex Shader
layout (location=0) in vec3 fragColour;

// Colore finale del fragment
layout (location=0) out vec4 outColour;

void main()
{
	outColour = vec4(fragColour, 1.0); // RGBA
	return;
}