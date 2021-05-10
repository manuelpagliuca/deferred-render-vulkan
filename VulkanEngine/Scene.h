#pragma once

#include "Utilities.h"
#include "DescriptorsHandler.h"
#include "CommandHandler.h"
#include "Mesh.h"

class Scene
{
public:
	Scene();
	Scene(MainDevice* mainDevice, DescriptorsHandler* descHandler, VkQueue* graphicsQueue, CommandHandler* commHandler);
	void LoadScene(std::vector<Mesh> &meshList, TextureObjects& textureObjects);

private:
	MainDevice*			m_MainDevice;
	DescriptorsHandler* m_DescriptorsHandler;
	VkQueue*			m_GraphicsQueue;
	CommandHandler*		m_CommandHandler;
};