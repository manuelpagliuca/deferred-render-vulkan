#version 450

#extension GL_KHR_vulkan_glsl: enable

#define NUM_LIGHTS 3

struct UboLight {
	vec3 	color;
	float 	ambient_intensity;
	vec3 	position;
	float 	radius;
};

layout(set = 0, binding = 0) uniform sampler2D inputPosition;
layout(set = 0, binding = 1) uniform sampler2D inputColour;
layout(set = 0, binding = 2) uniform sampler2D inputNormal;

layout(set = 1, binding = 0) uniform UboLights {
	UboLight l[NUM_LIGHTS]; 
} ubo_lights;

layout(set = 2, binding = 0) uniform SettingsData {
	int		render_target;
} settings;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 colour;

void main()
{
	colour = vec4(0.0);
	vec3 fragPos 	= texture(inputPosition, inUV.xy).rgb;
	vec3 fragColour = texture(inputColour, inUV.xy).rgb;
	vec3 fragNrm 	= texture(inputNormal, inUV.xy).rgb;
	gl_FragDepth 	= fragPos.z;

	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		vec3 L 				= ubo_lights.l[i].position.xyz - fragPos;

		float distance 		= length(L);
		float attenuation 	= ubo_lights.l[i].radius / (pow(distance, 2.0) + 1.0);

		vec3 View 		= vec3(0.0, 0.0, 0.0) - fragPos;

		// normalized values
		vec3 N 			= normalize(fragNrm);
		L 				= normalize(L);
		View 			= normalize(View);

		float dotNL 	= max(0.0, dot(N, L));
		vec3 diffuse  	= ubo_lights.l[i].color * fragColour * dotNL * attenuation;

		vec3 R 			= reflect(-L, N);
		float dotRV 	= max(0.0, dot(R, View));
		vec3 specular 	= ubo_lights.l[i].color * fragColour * pow(dotRV, 16.0) ; // * attenuation

		colour.rgb 	   += diffuse + specular;
	}
	
	switch(settings.render_target)
	{
	case 0:
		colour = vec4(fragPos, 1.0); 
		break;
	case 1:
		colour = vec4(fragNrm, 1.0); 
		break;
	case 2:
		colour = vec4(fragColour, 1.0); 
		break;
	case 3:
		break;
	}

	colour.a = 1.0;
}