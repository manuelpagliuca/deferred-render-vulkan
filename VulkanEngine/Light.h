#pragma once

#include "Utilities.h"

struct LightData {
	glm::vec3 m_Colour = glm::vec3(1.0f);
	float m_AmbientIntensity = 0.0f;
};

class Light
{
public:
	Light();
	Light(const float red, const float green, const float blue, const float ambient_intensity);
	~Light();

	LightData GetUBOData() const { return m_LightData; }

private:
	LightData m_LightData;
};