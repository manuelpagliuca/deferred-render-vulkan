#include "pch.h"

#include "Scene.h"

Scene::Scene()
{
	m_MainDevice			= nullptr;
	m_DescriptorsHandler	= nullptr;
	m_GraphicsQueue			= nullptr;
	m_CommandHandler		= nullptr;
}

Scene::Scene(MainDevice* mainDevice, DescriptorsHandler* descHandler, VkQueue* graphicsQueue, CommandHandler* commHandler)
{
	m_MainDevice			= mainDevice;
	m_DescriptorsHandler	= descHandler;
	m_GraphicsQueue			= graphicsQueue;
	m_CommandHandler		= commHandler;
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

	int giraffeTexture = Utility::CreateTexture(*m_MainDevice, m_DescriptorsHandler->GetTexturePool(),
		m_DescriptorsHandler->GetTextureDescriptorSetLayout(),
		textureObjects, *m_GraphicsQueue, m_CommandHandler->GetCommandPool(), "giraffe.jpg");

	meshList.push_back(Mesh(
		*m_MainDevice,
		*m_GraphicsQueue, m_CommandHandler->GetCommandPool(),
		&meshVertices, &meshIndices, giraffeTexture));

	meshList.push_back(Mesh(
		*m_MainDevice,
		*m_GraphicsQueue, m_CommandHandler->GetCommandPool(),
		&meshVertices2, &meshIndices, giraffeTexture));

	glm::mat4 meshModelMatrix = meshList[0].getModel().model;
	meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(45.f), glm::vec3(.0f, .0f, 1.0f));
	meshList[0].setModel(meshModelMatrix);
}
