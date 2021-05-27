#pragma once

#include "pch.h"

#include "GraphicPipeline.h"
#include "Mesh.h"
#include "MeshModel.h"

struct RecordObjects {
	TextureObjects TextureObjects;
};

class CommandHandler
{
public:
	CommandHandler();
	CommandHandler(MainDevice& mainDevice, GraphicPipeline* pipeline, VkRenderPass* renderPass);

	void CreateCommandPool(QueueFamilyIndices& queueIndices);
	void CreateCommandBuffers(size_t const numFrameBuffers);
	void RecordCommands(
		ImDrawData* draw_data, uint32_t currentImage, VkExtent2D& imageExtent,
		std::vector<VkFramebuffer>& frameBuffers, std::vector<Mesh>& meshList, std::vector<MeshModel>& modelList,
		TextureObjects& textureObjects, std::vector<VkDescriptorSet>& descriptorSets, std::vector<VkDescriptorSet>& inputDescriptorSet);

	void DestroyCommandPool();
	void FreeCommandBuffers();

	VkCommandPool& GetCommandPool()							{ return m_GraphicsComandPool; }
	VkCommandBuffer& GetCommandBuffer(uint32_t const index) { return m_CommandBuffers[index]; }
	std::vector<VkCommandBuffer>& GetCommandBuffers()		{ return m_CommandBuffers; }

private:
	MainDevice		m_MainDevice;

	VkRenderPass	*m_RenderPass;
	GraphicPipeline *m_GraphicPipeline;
	
	VkCommandPool	m_GraphicsComandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
};