#pragma once

#include "pch.h"

#include "Mesh.h"

struct Cube
{
	Cube();
	~Cube() = default;
	std::vector<uint32_t>& GetIndexData();
	std::vector<Vertex>& GetVertexData();
	glm::vec3& GetColor();
	void SetColor(const glm::vec3 color);

private:
	std::vector<Vertex> m_MeshVertices;
	std::vector<uint32_t> m_MeshIndices;
	glm::vec3 m_Color = glm::vec3(0.0f, 1.0f, 0.0f);
};