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
																// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Il CommandBuffer non pu� essere chiamato direttamente, ma da altri buffers via "vkCmdExecuteCommands".
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size()); // Numero di CommandBuffers da allocare

	VkResult res = vkAllocateCommandBuffers(m_MainDevice->LogicalDevice, &cbAllocInfo, m_CommandBuffers.data());

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate Command Buffers!");
}

void CommandHandler::RecordCommands(ImDrawData* draw_data, uint32_t currentImage, VkExtent2D &imageExtent, std::vector<VkFramebuffer> &frameBuffers,
	std::vector<Mesh> &meshList, std::vector<MeshModel>& modelList, TextureObjects & textureObjects, std::vector<VkDescriptorSet> &descriptorSets,
	std::vector<VkDescriptorSet> &inputDescriptorSet)
{
	VkCommandBufferBeginInfo buffer_begin_info = {};
	buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Il buffer pu� essere re-inviato al momento della resubmit

	VkRenderPassBeginInfo renderpass_begin_info = {};
	renderpass_begin_info.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderpass_begin_info.renderPass		= *m_RenderPass;		 
	renderpass_begin_info.renderArea.offset	= { 0, 0 };	 
	renderpass_begin_info.renderArea.extent	= imageExtent;

	// Valori che vengono utilizzati per pulire l'immagine (background color)
	std::array<VkClearValue, 3> clear_values;
	clear_values[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clear_values[1].color = { 0.0f, 0.0f, 0.1f, 0.8f };
	clear_values[2].depthStencil.depth = 1.0f;

	renderpass_begin_info.pClearValues = clear_values.data(); // Puntatore ad un array di ClearValues
	renderpass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());		   // Numero di ClearValues

	// Associo un Framebuffer per volta
	renderpass_begin_info.framebuffer = frameBuffers[currentImage];

	// Inizia a registrare i comandi nel Command Buffer
	VkResult res = vkBeginCommandBuffer(m_CommandBuffers[currentImage], &buffer_begin_info);

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to start recording a Command Buffer!");

	// Inizio del RenderPass
	// VK_SUBPASS_CONTENTS_INLINE indica che tutti i comandi verranno registrati direttamente sul CommandBuffer primario
	// e che il CommandBuffer secondario non debba essere eseguito all'interno del Subpass
	vkCmdBeginRenderPass(m_CommandBuffers[currentImage], &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);


	// Binding della Pipeline Grafica ad un Command Buffer.
	// VK_PIPELINE_BIND_POINT_GRAPHICS, indica il tipo Binding Point della Pipeline (in questo grafico per la grafica).
	// (� possibile impostare molteplici pipeline e switchare, per esempio una versione DEFERRED o WIREFRAME)
	vkCmdBindPipeline(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline->GetPipeline());

	for (size_t j = 0; j < modelList.size(); ++j)
	{
		MeshModel mesh_model = modelList[j];
		//Model m = mesh_model.GetModel();// meshList[j].getModel();
		Model m;
		m.model = mesh_model.GetModel();

		vkCmdPushConstants(m_CommandBuffers[currentImage], m_GraphicPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Model), &m);

		for (size_t k = 0; k < mesh_model.GetMeshCount(); k++)
		{

			VkBuffer vertexBuffers[] = { mesh_model.GetMesh(k)->getVertexBuffer() }; // Buffer da bindare prima di essere disegnati
			VkDeviceSize offsets[] = { 0 };

			// Binding del Vertex Buffer di una Mesh ad un Command Buffer
			vkCmdBindVertexBuffers(m_CommandBuffers[currentImage], 0, 1, vertexBuffers, offsets);

			// Bind del Index Buffer di una Mesh ad un Command Buffer, con offset 0 ed usando uint_32
			vkCmdBindIndexBuffer(m_CommandBuffers[currentImage], mesh_model.GetMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			std::array<VkDescriptorSet, 2> descriptorSetGroup = {
				descriptorSets[currentImage],
				textureObjects.SamplerDescriptorSets[mesh_model.GetMesh(k)->getTexID()]
			};

			// Binding del Descriptor Set ad un Command Buffer DYNAMIC UBO
			//uint32_t dynamicOffset = static_cast<uint32_t>(m_modelUniformAlignment) * static_cast<uint32_t>(j);

			vkCmdBindDescriptorSets(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_GraphicPipeline->GetLayout(), 0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);

			// Invia un Comando di IndexedDraw ad un Command Buffer
			vkCmdDrawIndexed(m_CommandBuffers[currentImage], mesh_model.GetMesh(k)->getIndexCount(), 1, 0, 0, 0);
		}
	}

	// Start second subpass
	vkCmdNextSubpass(m_CommandBuffers[currentImage], VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline->GetSecondPipeline());

	vkCmdBindDescriptorSets(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_GraphicPipeline->GetSecondLayout(), 0, 1, &inputDescriptorSet[currentImage], 0, nullptr);
	vkCmdDraw(m_CommandBuffers[currentImage], 3, 1, 0, 0);
		
	/*

	VkBuffer vertexBuffers[] = { meshList[j].getVertexBuffer() }; // Buffer da bindare prima di essere disegnati
	VkDeviceSize offsets[] = { 0 };

	// Binding del Vertex Buffer di una Mesh ad un Command Buffer
	vkCmdBindVertexBuffers(m_CommandBuffers[currentImage], 0, 1, vertexBuffers, offsets);

	// Bind del Index Buffer di una Mesh ad un Command Buffer, con offset 0 ed usando uint_32
	vkCmdBindIndexBuffer(m_CommandBuffers[currentImage], meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

	std::array<VkDescriptorSet, 2> descriptorSetGroup = { descriptorSets[currentImage], textureObjects.SamplerDescriptorSets[meshList[j].getTexID()] };

	// Binding del Descriptor Set ad un Command Buffer DYNAMIC UBO
	//uint32_t dynamicOffset = static_cast<uint32_t>(m_modelUniformAlignment) * static_cast<uint32_t>(j);

	vkCmdBindDescriptorSets(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_GraphicPipeline->GetLayout(), 0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);

	// Invia un Comando di IndexedDraw ad un Command Buffer
	vkCmdDrawIndexed(m_CommandBuffers[currentImage], meshList[j].getIndexCount(), 1, 0, 0, 0);
	*/
	

	vkCmdEndRenderPass(m_CommandBuffers[currentImage]);
	//ImGui_ImplVulkan_RenderDrawData(draw_data, m_CommandBuffers[currentImage]);

	res = vkEndCommandBuffer(m_CommandBuffers[currentImage]);

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