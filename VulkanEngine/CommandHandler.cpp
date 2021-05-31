#include "pch.h"

#include "CommandHandler.h"

CommandHandler::CommandHandler()
{
	m_MainDevice			= {};
	m_GraphicsComandPool	= {};
	m_CommandBuffers		= {};
	m_GraphicPipeline		= nullptr;
}

CommandHandler::CommandHandler(MainDevice* mainDevice, GraphicPipeline *pipeline, VkRenderPass* renderPass)
{
	m_MainDevice			= mainDevice;
	m_GraphicPipeline		= pipeline;
	m_RenderPass			= renderPass;
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

void CommandHandler::RecordCommands(
	ImDrawData* draw_data, uint32_t current_img, VkExtent2D &imageExtent, 
	std::vector<VkFramebuffer> &frameBuffers,
	std::vector<Mesh> &meshList, 
	std::vector<MeshModel> &modelList, 
	TextureObjects &tex_objects,
	std::vector<VkDescriptorSet> &vp_desc_sets, 
	std::vector<VkDescriptorSet> &light_desc_sets,
	std::vector<VkDescriptorSet> &inputDescriptorSet)
{
	VkCommandBufferBeginInfo buffer_begin_info = {};
	buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Il buffer può essere re-inviato al momento della resubmit

	std::array<VkClearValue, 3> clear_values;
	clear_values[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clear_values[1].color = { 0.0f, 0.0f, 0.1f, 0.8f };
	clear_values[2].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo renderpass_begin_info = {};
	renderpass_begin_info.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderpass_begin_info.renderPass		= *m_RenderPass;		 
	renderpass_begin_info.renderArea.offset	= { 0, 0 };	 
	renderpass_begin_info.renderArea.extent	= imageExtent;
	renderpass_begin_info.pClearValues		= clear_values.data(); 
	renderpass_begin_info.clearValueCount	= static_cast<uint32_t>(clear_values.size());	
	renderpass_begin_info.framebuffer		= frameBuffers[current_img];

	VkResult res = vkBeginCommandBuffer(m_CommandBuffers[current_img], &buffer_begin_info);

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to start recording a Command Buffer!");

	vkCmdBeginRenderPass(m_CommandBuffers[current_img], &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_CommandBuffers[current_img], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline->GetPipeline());

	for (size_t j = 0; j < modelList.size(); ++j)
	{
		MeshModel mesh_model = modelList[j];
		Model m;
		m.model = mesh_model.GetModel();

		vkCmdPushConstants(m_CommandBuffers[current_img], m_GraphicPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Model), &m);

		for (size_t k = 0; k < mesh_model.GetMeshCount(); k++)
		{
			VkBuffer vertexBuffers[] = { mesh_model.GetMesh(k)->getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(m_CommandBuffers[current_img], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(m_CommandBuffers[current_img], mesh_model.GetMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			std::array<VkDescriptorSet, 3> desc_set_group = {
				vp_desc_sets[current_img],
				tex_objects.SamplerDescriptorSets[mesh_model.GetMesh(k)->getTexID()],
				light_desc_sets[current_img]
			};

			vkCmdBindDescriptorSets(m_CommandBuffers[current_img], VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_GraphicPipeline->GetLayout(), 0, static_cast<uint32_t>(desc_set_group.size()), desc_set_group.data(), 0, nullptr);

			vkCmdDrawIndexed(m_CommandBuffers[current_img], mesh_model.GetMesh(k)->getIndexCount(), 1, 0, 0, 0);
		}
	}

	vkCmdNextSubpass(m_CommandBuffers[current_img], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffers[current_img], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline->GetSecondPipeline());

		vkCmdBindDescriptorSets(m_CommandBuffers[current_img], VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_GraphicPipeline->GetSecondLayout(), 0, 1, &inputDescriptorSet[current_img], 0, nullptr);
		vkCmdDraw(m_CommandBuffers[current_img], 3, 1, 0, 0);
	
	vkCmdEndRenderPass(m_CommandBuffers[current_img]);
	
	//ImGui_ImplVulkan_RenderDrawData(draw_data, m_CommandBuffers[currentImage]);

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