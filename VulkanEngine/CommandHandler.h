#pragma once

#include "pch.h"

#include "GraphicPipeline.h"
#include "Mesh.h"

struct RecordObjects {
	TextureObjects TextureObjects;
};

class CommandHandler
{
public:
	CommandHandler();
	CommandHandler(MainDevice& mainDevice, GraphicPipeline& pipeline, VkRenderPass& renderPass);

	void CreateCommandPool(QueueFamilyIndices& queueIndices);
	void CreateCommandBuffers(size_t const numFrameBuffers);
	void RecordCommands(uint32_t currentImage, VkExtent2D& imageExtent, std::vector<VkFramebuffer>& frameBuffers,
		std::vector<Mesh>& meshList, TextureObjects& textureObjects, std::vector<VkDescriptorSet>& descriptorSets);

	void DestroyCommandPool();

	VkCommandPool& GetCommandPool() { return m_GraphicsComandPool; }
	VkCommandBuffer& GetCommandBuffer(uint32_t const index) { return m_CommandBuffers[index]; }
	std::vector<VkCommandBuffer>& GetCommandBuffers() { return m_CommandBuffers; }

private:
	MainDevice m_MainDevice;
	GraphicPipeline m_GraphicPipeline;
	VkRenderPass m_RenderPass;
	VkCommandPool m_GraphicsComandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
};