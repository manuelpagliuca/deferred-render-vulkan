#include "pch.h"

#include "Scene.h"
#include "Cube.h"

Scene::Scene()
{
	m_RenderData = {};
}

Scene::Scene(const VulkanRenderData &render_data)
{
	m_RenderData		 = render_data;
}

void Scene::PassRenderData(const VulkanRenderData& render_data)
{
	m_RenderData = render_data;
}

void Scene::LoadScene(std::vector<Mesh> &meshList, TextureObjects &textureObjects)
{
	std::vector<Vertex> meshVertices =
	{
		{ { -0.4,  0.4,  0.0 }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
		{ { -0.4, -0.4,  0.0 }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
		{ {  0.4, -0.4,  0.0 }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
		{ {  0.4,  0.4,  0.0 }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	};

	std::vector<Vertex> meshVertices2 = {
		{ { -0.25,  0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -0.25, -0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ {  0.25, -0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ {  0.25,  0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	};

	std::vector<uint32_t> meshIndices = {
		0, 1, 2,
		2, 3, 0
	};

	int giraffeTexture = TextureLoader::GetInstance()->CreateTexture("giraffe.jpg");

	meshList.push_back(Mesh(
		m_RenderData.main_device,
		m_RenderData.graphic_queue, m_RenderData.command_pool,
		&meshVertices, &meshIndices, giraffeTexture));

	meshList.push_back(Mesh(
		m_RenderData.main_device,
		m_RenderData.graphic_queue, m_RenderData.command_pool,
		&meshVertices2, &meshIndices, giraffeTexture));

	Cube cube;
	meshList.push_back(Mesh(m_RenderData.main_device,
		m_RenderData.graphic_queue, m_RenderData.command_pool,
		&cube.GetVertexData(), &cube.GetIndexData(), NULL));

	glm::mat4 meshModelMatrix = meshList[0].getModel().model;
	meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(45.f), glm::vec3(.0f, .0f, 1.0f));
	meshList[0].setModel(meshModelMatrix);
}