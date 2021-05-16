#pragma once

#include "Utilities.h"
#include "DescriptorsHandler.h"
#include "CommandHandler.h"
#include "Mesh.h"

class Scene
{
public:
	Scene();
	Scene(const VulkanRenderData& render_data, DescriptorsHandler* descriptor_handler);

	void PassRenderData(const VulkanRenderData& render_data, DescriptorsHandler* descriptor_handler);

	void LoadScene(std::vector<Mesh> &mesh_list, TextureObjects& texture_objects);

private:
	VulkanRenderData	m_RenderData;
	DescriptorsHandler* m_DescriptorsHandler;
};