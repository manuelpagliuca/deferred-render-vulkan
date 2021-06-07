#include "pch.h"

#include "CommandHandler.h"

CommandHandler::CommandHandler()
{
	m_MainDevice			= {};
	m_GraphicsComandPool	= {};
	m_CommandBuffers		= {};
	m_GraphicPipeline		= nullptr;
}

CommandHandler::CommandHandler(MainDevice* mainDevice, GraphicPipeline *pipeline, RenderPassHandler* renderPassHandler)
{
	m_MainDevice			= mainDevice;
	m_GraphicPipeline		= pipeline;
	m_RenderPassHandler		= renderPassHandler;
	m_GraphicsComandPool	= {};
	m_CommandBuffers		= {};
}

void CommandHandler::CreateCommandPool(QueueFamilyIndices &queueIndices)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Il nostro CommandBuffer viene resettato ogni qualvolta si accede alla 'vkBeginCommandBuffer()'
	poolInfo.queueFamilyIndex	= queueIndices.GraphicsFamily;			 // I comandi dei CommandBuffers funzionano solo per le Graphic Queues

	VkResult res = vkCreateCommandPool(m_MainDevice->LogicalDevice, &poolInfo, nullptr, &m_GraphicsComandPool);

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to create a Command Pool!");
}

void CommandHandler::CreateCommandBuffers(size_t const numFrameBuffers)
{
	m_CommandBuffers.resize(numFrameBuffers);

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = m_GraphicsComandPool;				// Command Pool dalla quale allocare il Command Buffer
	cbAllocInfo.level		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// Livello del Command Buffer
																// VK_COMMAND_BUFFER_LEVEL_PRIMARY   : Il CommandBuffer viene inviato direttamente sulla queue. 
																// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Il CommandBuffer non può essere chiamato direttamente, ma da altri buffers via "vkCmdExecuteCommands".
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size()); // Numero di CommandBuffers da allocare

	VkResult res = vkAllocateCommandBuffers(m_MainDevice->LogicalDevice, &cbAllocInfo, m_CommandBuffers.data());

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate Command Buffers!");
}

void CommandHandler::RecordOffScreenCommands(ImDrawData* draw_data, uint32_t currentImage, VkExtent2D& imageExtent,
	std::vector<VkFramebuffer>& offScreenFrameBuffers, std::vector<Mesh>& meshList, std::vector<MeshModel>& modelList,
	TextureObjects& textureObjects, std::vector<VkDescriptorSet>& descriptorSets, std::vector<VkDescriptorSet>& inputDescriptorSet,
	std::vector<BufferImage>& position_image, std::vector<BufferImage>& colour_image, std::vector<BufferImage>& normal_image, QueueFamilyIndices queueFamilyIndices)
{
	VkCommandBufferBeginInfo buffer_begin_info = {};
	buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Il buffer può essere re-inviato al momento della resubmit

	std::array<VkClearValue, 4> clear_values;
	clear_values[0].color = { 0.0f, 0.0f, 0.0f, 0.0f }; // Position
	clear_values[1].color = { 0.0f, 0.0f, 0.0f, 0.0f }; // Colour
	clear_values[2].color = { 0.0f, 0.0f, 0.0f, 0.0f }; // Normal
	clear_values[3].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo renderpass_begin_info = {};
	renderpass_begin_info.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderpass_begin_info.renderPass		= *m_RenderPassHandler->GetOffScreenRenderPassReference();		 
	renderpass_begin_info.renderArea.offset	= { 0, 0 };	 
	renderpass_begin_info.renderArea.extent	= imageExtent;
	renderpass_begin_info.pClearValues		= clear_values.data(); 
	renderpass_begin_info.clearValueCount	= static_cast<uint32_t>(clear_values.size());	
	renderpass_begin_info.framebuffer		= offScreenFrameBuffers[currentImage];

	VkResult res = vkBeginCommandBuffer(m_CommandBuffers[currentImage], &buffer_begin_info);

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to start recording a Command Buffer!");

	// Offscreen render pass
	vkCmdBeginRenderPass(m_CommandBuffers[currentImage], &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline->GetPipeline());

	for (size_t j = 0; j < modelList.size(); ++j)
	{
		MeshModel mesh_model = modelList[j];
		Model m;
		m.model = mesh_model.GetModel();

		vkCmdPushConstants(m_CommandBuffers[currentImage], m_GraphicPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Model), &m);

		for (size_t k = 0; k < mesh_model.GetMeshCount(); k++)
		{
			VkBuffer vertexBuffers[] = { mesh_model.GetMesh(k)->getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(m_CommandBuffers[currentImage], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(m_CommandBuffers[currentImage], mesh_model.GetMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			std::array<VkDescriptorSet, 2> desc_set_group = {
				descriptorSets[currentImage],
				textureObjects.SamplerDescriptorSets[mesh_model.GetMesh(k)->getTexID()]
			};

			vkCmdBindDescriptorSets(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_GraphicPipeline->GetLayout(), 0, static_cast<uint32_t>(desc_set_group.size()), desc_set_group.data(), 0, nullptr);

			vkCmdDrawIndexed(m_CommandBuffers[currentImage], mesh_model.GetMesh(k)->getIndexCount(), 1, 0, 0, 0);
		}
	}

	vkCmdEndRenderPass(m_CommandBuffers[currentImage]);

	res = vkEndCommandBuffer(m_CommandBuffers[currentImage]);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to stop recording offscreen Command Buffer!");
	}
}

void CommandHandler::RecordCommands(
	ImDrawData* draw_data, uint32_t current_img, VkExtent2D& imageExtent,
	std::vector<VkFramebuffer>& frameBuffers,
	std::vector<VkDescriptorSet>& light_desc_sets,
	std::vector<VkDescriptorSet>& inputDescriptorSet)
{
	VkCommandBufferBeginInfo buffer_begin_info = {};
	buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Il buffer può essere re-inviato al momento della resubmit

	std::array<VkClearValue, 2> clear_values;
	clear_values[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clear_values[1].depthStencil.depth = 1.0f;
	
	VkRenderPassBeginInfo renderpass_begin_info = {};
	renderpass_begin_info.sType					= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderpass_begin_info.renderPass			= *m_RenderPassHandler->GetRenderPassReference();
	renderpass_begin_info.renderArea.offset		= { 0, 0 };
	renderpass_begin_info.renderArea.extent		= imageExtent;
	renderpass_begin_info.pClearValues			= clear_values.data();
	renderpass_begin_info.clearValueCount		= static_cast<uint32_t>(clear_values.size());
	renderpass_begin_info.framebuffer			= frameBuffers[current_img];

	VkResult res = vkBeginCommandBuffer(m_CommandBuffers[current_img], &buffer_begin_info);

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to start recording the Command Buffer for the presentation!"); 

	// View render pass
	vkCmdBeginRenderPass(m_CommandBuffers[current_img], &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_CommandBuffers[current_img], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline->GetSecondPipeline());

	{
		std::array<VkDescriptorSet, 2> desc_set_group =
		{
			inputDescriptorSet[current_img],
			light_desc_sets[current_img]
		};

		vkCmdBindDescriptorSets(
			m_CommandBuffers[current_img], 
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_GraphicPipeline->GetSecondLayout(), 0, 
			static_cast<uint32_t>(desc_set_group.size()), desc_set_group.data(), 0, nullptr);
	}

	vkCmdDraw(m_CommandBuffers[current_img], 3, 1, 0, 0);

	ImGui_ImplVulkan_RenderDrawData(draw_data, m_CommandBuffers[current_img]);

	vkCmdEndRenderPass(m_CommandBuffers[current_img]);

	res = vkEndCommandBuffer(m_CommandBuffers[current_img]);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to stop recording a Command Buffer!");
	}
}

void CommandHandler::DestroyCommandPool()
{
	vkDestroyCommandPool(m_MainDevice->LogicalDevice, m_GraphicsComandPool, nullptr);
}

void CommandHandler::FreeCommandBuffers()
{
	vkFreeCommandBuffers(m_MainDevice->LogicalDevice, m_GraphicsComandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
}