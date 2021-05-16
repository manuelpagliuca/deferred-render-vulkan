#include "pch.h"

#include "Scene.h"

Scene::Scene()
{
	m_RenderData = {};
	m_DescriptorsHandler	= nullptr;
}

Scene::Scene(const VulkanRenderData &render_data, DescriptorsHandler* descriptor_handler)
{
	m_RenderData		 = render_data;
	m_DescriptorsHandler = descriptor_handler;
}

void Scene::PassRenderData(const VulkanRenderData& render_data, DescriptorsHandler* descriptor_handler)
{
	m_RenderData = render_data;
	m_DescriptorsHandler = descriptor_handler;
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

	int giraffeTexture = Utility::CreateTexture(m_RenderData.main_device, m_DescriptorsHandler->GetTexturePool(),
		m_DescriptorsHandler->GetTextureDescriptorSetLayout(),
		textureObjects, m_RenderData.graphic_queue, m_RenderData.command_pool, "giraffe.jpg");

	meshList.push_back(Mesh(
		m_RenderData.main_device,
		m_RenderData.graphic_queue, m_RenderData.command_pool,
		&meshVertices, &meshIndices, giraffeTexture));

	meshList.push_back(Mesh(
		m_RenderData.main_device,
		m_RenderData.graphic_queue, m_RenderData.command_pool,
		&meshVertices2, &meshIndices, giraffeTexture));

	glm::mat4 meshModelMatrix = meshList[0].getModel().model;
	meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(45.f), glm::vec3(.0f, .0f, 1.0f));
	meshList[0].setModel(meshModelMatrix);
}
