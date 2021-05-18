#pragma once

#include "pch.h"

#include "Cube.h"

Cube::Cube()
{
	m_MeshVertices = {		
		{ { -0.5f,  0.5f,  0.5f }, m_Color, { 1.0f, 1.0f } },
		{ { -0.5f, -0.5f,  0.5f }, m_Color, { 1.0f, 0.0f } },
		{ {  0.5f, -0.5f,  0.5f }, m_Color, { 0.0f, 0.0f } },
		{ {  0.5f,  0.5f,  0.5f }, m_Color, { 0.0f, 1.0f } },

		{ {  0.5f,  0.5f, -0.5f }, m_Color, { 0.0f, 1.0f } },
		{ {  0.5f, -0.5f, -0.5f }, m_Color, { 0.0f, 1.0f } },
		
		{ { -0.5f,  0.5f, -0.5f }, m_Color, { 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f }, m_Color, { 0.0f, 1.0f } }
	};

	m_MeshIndices = {
		0, 1, 2,
		2, 3, 0,

		4, 3, 2,
		5, 4, 2,

		6, 4, 5,
		7, 6, 5,

		6, 7, 0,
		1, 0, 7,

		3, 6, 0,
		4, 6, 3,

		1, 7, 2,
		7, 5, 2
	};

}

std::vector<uint32_t>& Cube::GetIndexData()
{
	return m_MeshIndices;
}

std::vector<Vertex>& Cube::GetVertexData()
{
	return m_MeshVertices;
}

glm::vec3& Cube::GetColor()
{
	return m_Color;
}

void Cube::SetColor(const glm::vec3 color)
{
	m_Color = color;
}
