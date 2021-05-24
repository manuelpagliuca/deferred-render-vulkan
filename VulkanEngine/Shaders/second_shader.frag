#version 450

// La definizione del binding rimane obbligatoria
layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColour;
layout(input_attachment_index = 0, binding = 1) uniform subpassInput inputDepth;

layout(location = 0) out vec4 colour;

void main()
{
	colour = subpassLoad(inputColour).rgba;
}

