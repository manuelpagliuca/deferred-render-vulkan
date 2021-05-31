#pragma once

#include "Utilities.h"
#include "DescriptorsHandler.h"
#include "CommandHandler.h"
#include "Mesh.h"
#include "TextureLoader.h"
#include "MeshModel.h"

class Scene
{
public:
	Scene();
	Scene(const VulkanRenderData& render_data);

	void PassRenderData(const VulkanRenderData& render_data);

	void LoadScene(std::vector<Mesh> &mesh_list, TextureObjects& texture_objects);

private:
	VulkanRenderData	m_RenderData;
};