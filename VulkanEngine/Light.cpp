#include "pch.h"
#include "Light.h"

Light::Light()
{
	m_LightData.m_Colour			= glm::vec3(1.0f, 1.0f, 1.0f);
	m_LightData.m_AmbientIntensity	= 1.0f;
}

Light::Light(const float red, const float green, const float blue, const float ambient_intensity)
{
	m_LightData.m_Colour.x = red;
	m_LightData.m_Colour.y = green;
	m_LightData.m_Colour.z = blue;

	m_LightData.m_AmbientIntensity	= ambient_intensity;
}

Light::~Light()
{

}