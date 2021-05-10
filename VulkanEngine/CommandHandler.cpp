#include "pch.h"

#include "CommandHandler.h"

CommandHandler::CommandHandler()
{
	m_MainDevice			= {};
	m_GraphicsComandPool	= {};
	m_CommandBuffers		= {};
	m_GraphicPipeline		= nullptr;
}

CommandHandler::CommandHandler(MainDevice& mainDevice, GraphicPipeline *pipeline, VkRenderPass* renderPass)
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
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Il nostro CommandBuffer viene resettato ogni qualvolta si accede alla 'vkBeginCommandBuffer()'
	poolInfo.queueFamilyIndex = queueIndices.GraphicsFamily;			 // I comandi dei CommandBuffers funzionano solo per le Graphic Queues

	VkResult res = vkCreateCommandPool(m_MainDevice.LogicalDevice, &poolInfo, nullptr, &m_GraphicsComandPool);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Command Pool!");
	}
}

// Allocazione dei Command Buffers (gruppi di comandi da inviare alle queues)
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

	VkResult res = vkAllocateCommandBuffers(m_MainDevice.LogicalDevice, &cbAllocInfo, m_CommandBuffers.data());

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Command Buffers!");
	}
}

void CommandHandler::RecordCommands(uint32_t currentImage, VkExtent2D &imageExtent, std::vector<VkFramebuffer> &frameBuffers, 
	std::vector<Mesh> &meshList, TextureObjects & textureObjects, std::vector<VkDescriptorSet> &descriptorSets)
{
	// Informazioni sul come inizializzare ogni Command Buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Il buffer può essere re-inviato al momento della resubmit

	// Informazioni sul come inizializzare il RenderPass (necessario solo per le applicazioni grafiche)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = *m_RenderPass;		 // RenderPass
	renderPassBeginInfo.renderArea.offset = { 0, 0 };	 // Start point del RenderPass (in Pixels)
	renderPassBeginInfo.renderArea.extent = imageExtent; // Dimensione delal regione dove eseguire il RenderPass (partendo dall'offset)

	// Valori che vengono utilizzati per pulire l'immagine (background color)
	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = { 0.6f, 0.65f, 0.4f, 1.0f };
	clearValues[1].depthStencil.depth = 1.0f;

	renderPassBeginInfo.pClearValues = clearValues.data(); // Puntatore ad un array di ClearValues
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());		   // Numero di ClearValues

	// Associo un Framebuffer per volta
	renderPassBeginInfo.framebuffer = frameBuffers[currentImage];

	// Inizia a registrare i comandi nel Command Buffer
	VkResult res = vkBeginCommandBuffer(m_CommandBuffers[currentImage], &bufferBeginInfo);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to start recording a Command Buffer!");
	}

	// Inizio del RenderPass
	// VK_SUBPASS_CONTENTS_INLINE indica che tutti i comandi verranno registrati direttamente sul CommandBuffer primario
	// e che il CommandBuffer secondario non debba essere eseguito all'interno del Subpass
	vkCmdBeginRenderPass(m_CommandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Binding della Pipeline Grafica ad un Command Buffer.
	// VK_PIPELINE_BIND_POINT_GRAPHICS, indica il tipo Binding Point della Pipeline (in questo grafico per la grafica).
	// (È possibile impostare molteplici pipeline e switchare, per esempio una versione DEFERRED o WIREFRAME)

	vkCmdBindPipeline(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline->GetPipeline());

	for (size_t j = 0; j < meshList.size(); ++j)
	{
		VkBuffer vertexBuffers[] = { meshList[j].getVertexBuffer() }; // Buffer da bindare prima di essere disegnati
		VkDeviceSize offsets[] = { 0 };

		// Binding del Vertex Buffer di una Mesh ad un Command Buffer
		vkCmdBindVertexBuffers(m_CommandBuffers[currentImage], 0, 1, vertexBuffers, offsets);

		// Bind del Index Buffer di una Mesh ad un Command Buffer, con offset 0 ed usando uint_32
		vkCmdBindIndexBuffer(m_CommandBuffers[currentImage], meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

		Model m = meshList[j].getModel();

		vkCmdPushConstants(m_CommandBuffers[currentImage], m_GraphicPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Model), &m);

		std::array<VkDescriptorSet, 2> descriptorSetGroup = { descriptorSets[currentImage], textureObjects.SamplerDescriptorSets[meshList[j].getTexID()] };

		// Binding del Descriptor Set ad un Command Buffer DYNAMIC UBO
		//uint32_t dynamicOffset = static_cast<uint32_t>(m_modelUniformAlignment) * static_cast<uint32_t>(j);

		vkCmdBindDescriptorSets(m_CommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_GraphicPipeline->GetLayout(), 0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);

		// Invia un Comando di IndexedDraw ad un Command Buffer
		vkCmdDrawIndexed(m_CommandBuffers[currentImage], meshList[j].getIndexCount(), 1, 0, 0, 0);
	}

	// Termine del Render Pass
	vkCmdEndRenderPass(m_CommandBuffers[currentImage]);

	// Termian la registrazione dei comandi
	res = vkEndCommandBuffer(m_CommandBuffers[currentImage]);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to stop recording a Command Buffer!");
	}
}

void CommandHandler::DestroyCommandPool()
{
	vkDestroyCommandPool(m_MainDevice.LogicalDevice, m_GraphicsComandPool, nullptr);
}

void CommandHandler::FreeCommandBuffers()
{
	vkFreeCommandBuffers(m_MainDevice.LogicalDevice, m_GraphicsComandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
}