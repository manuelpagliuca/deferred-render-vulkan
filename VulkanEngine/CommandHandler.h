#pragma once

#include "pch.h"

#include "GraphicPipeline.h"
#include "RenderPassHandler.h"
#include "Mesh.h"
#include "MeshModel.h"

struct RecordObjects {
	TextureObjects TextureObjects;
};

class CommandHandler
{
public:
	CommandHandler();
	CommandHandler(MainDevice* main_device, GraphicPipeline* pipeline, RenderPassHandler* renderPassHandler);

	void CreateCommandPool(QueueFamilyIndices& queueIndices);
	void CreateCommandBuffers(size_t const numFrameBuffers);
	void RecordOffScreenCommands(ImDrawData* draw_data, uint32_t currentImage, VkExtent2D& imageExtent, 
		std::vector<VkFramebuffer>& offScreenFrameBuffers, std::vector<Mesh>& meshList, std::vector<MeshModel>& modelList,
		TextureObjects& textureObjects, std::vector<VkDescriptorSet>& descriptorSets, std::vector<VkDescriptorSet>& inputDescriptorSet,
		std::vector<BufferImage>& position_image, std::vector<BufferImage>& colour_image, std::vector<BufferImage>& normal_image, QueueFamilyIndices queueFamilyIndices);
	void RecordCommands(ImDrawData* draw_data, uint32_t current_img, VkExtent2D& imageExtent,
		std::vector<VkFramebuffer>& frameBuffers,
		std::vector<VkDescriptorSet>& light_desc_sets,
		std::vector<VkDescriptorSet>& inputDescriptorSet);

	void DestroyCommandPool();
	void FreeCommandBuffers();

	VkCommandPool& GetCommandPool()							{ return m_GraphicsComandPool; }
	VkCommandBuffer& GetCommandBuffer(uint32_t const index) { return m_CommandBuffers[index]; }
	std::vector<VkCommandBuffer>& GetCommandBuffers()		{ return m_CommandBuffers; }

private:
	MainDevice			*m_MainDevice;
	RenderPassHandler	*m_RenderPassHandler;
	GraphicPipeline		*m_GraphicPipeline;
	
	VkCommandPool	m_GraphicsComandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
};