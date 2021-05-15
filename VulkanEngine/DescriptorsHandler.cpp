#include "pch.h"
#include "Utilities.h"
#include "DescriptorsHandler.h"
#include "imgui.h"

DescriptorsHandler::DescriptorsHandler()
{
	m_ViewProjectionLayout  = {};
	m_Device				= {};
	m_ViewProjectionPool	= {};
	m_ViewProjectionLayout	= {};
	m_TexturePool			= {};
	m_TextureLayout			= {};

	std::vector<VkDescriptorSet> m_DescriptorSets = {};
}

DescriptorsHandler::DescriptorsHandler(VkDevice *device)
{
	m_Device = device;
}

void DescriptorsHandler::CreateDescriptorPool(size_t numOfSwapImgs, size_t UBOsize)
{
	/* CREATE UNIFORM DESCRIPTOR POOL */

	// VIEW - PROJECTION
	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;			   // Che tipo di descriptor contiene (non è un descriptor set ma sono quelli presenti negli shaders)
	vpPoolSize.descriptorCount = static_cast<uint32_t> (UBOsize); // Quanti descriptor

	// MODEL (DYNAMIC UBO)
	/*VkDescriptorPoolSize modelPoolSize = {};
	modelPoolSize.type			  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelPoolSize.descriptorCount = static_cast<uint32_t>(m_modelDynamicUBO.size()); */

	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

	// Data per la creazione della pool
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(numOfSwapImgs); // un descriptor set per ogni commandbuffer/swapimage
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());											  // Quante pool
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();		// Array di pool size

	// Creazione della Descriptor Pool
	VkResult result = vkCreateDescriptorPool(*m_Device, &poolCreateInfo, nullptr, &m_ViewProjectionPool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}

	/* TEXTURE SAMPLER DESCRIPTOR POOL */

	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = MAX_OBJECTS;

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
	samplerPoolCreateInfo.poolSizeCount = 1;
	samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

	result = vkCreateDescriptorPool(*m_Device, &samplerPoolCreateInfo, nullptr, &m_TexturePool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}

	/* IMGUI DESCRIPTOR POOL */
	VkDescriptorPoolSize imguiPoolSize[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo imgui_pool = {};
	imgui_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	imgui_pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	imgui_pool.maxSets = 1000 * IM_ARRAYSIZE(imguiPoolSize);
	imgui_pool.poolSizeCount = (uint32_t)IM_ARRAYSIZE(imguiPoolSize);
	imgui_pool.pPoolSizes = imguiPoolSize;

	result = vkCreateDescriptorPool(*m_Device, &imgui_pool, nullptr, &m_ImguiDescriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}
}

void DescriptorsHandler::CreateViewProjectionDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding viewProjectionLayoutBinding = {};
	viewProjectionLayoutBinding.binding				= 0;									
	viewProjectionLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
	viewProjectionLayoutBinding.descriptorCount		= 1;								
	viewProjectionLayoutBinding.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT;		
	viewProjectionLayoutBinding.pImmutableSamplers	= nullptr;						

	// Model Binding Info (DYNAMIC UBO)
	/*
	VkDescriptorSetLayoutBinding modelLayoutBinding = {};
	modelLayoutBinding.binding			   = 1;											// Punto di binding nello Shader
	modelLayoutBinding.descriptorType	   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // Tipo di Descriptor Set (Dynamic Uniform Buffer Object)
	modelLayoutBinding.descriptorCount	   = 1;											// Numero di Descriptor Set da bindare
	modelLayoutBinding.stageFlags		   = VK_SHADER_STAGE_VERTEX_BIT;				// Shaders Stage nel quale viene effettuato il binding
	modelLayoutBinding.pImmutableSamplers  = nullptr;									// rendere il sampler immutable specificando il layout, serve per le textures
	*/
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { viewProjectionLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount	= static_cast<uint32_t>(layoutBindings.size());
	layoutCreateInfo.pBindings		= layoutBindings.data();

	VkResult result = vkCreateDescriptorSetLayout(*m_Device, &layoutCreateInfo, nullptr, &m_ViewProjectionLayout);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create a Descriptor Set Layout");
}

void DescriptorsHandler::CreateTextureDescriptorSetLayout()
{
	// Create texture sampler descriptor set layout
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
	textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutCreateInfo.bindingCount = 1;
	textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

	VkResult result = vkCreateDescriptorSetLayout(*m_Device, &textureLayoutCreateInfo, nullptr, &m_TextureLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Set Layout!");
	}
}

void DescriptorsHandler::CreateDescriptorSets(std::vector<VkBuffer> & viewProjectionUBO, size_t dataSize, size_t numSwapChainImgs)
{
	// Un Descriptor Set per ogni Uniform Buffer
	m_DescriptorSets.resize(numSwapChainImgs);

	// Lista di tutti i possibili layour che useremo dal set (?) non capito TODO
	std::vector<VkDescriptorSetLayout> setLayouts(numSwapChainImgs, m_ViewProjectionLayout);

	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool		= m_ViewProjectionPool;								 // Pool per l'allocazione dei Descriptor Set
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(numSwapChainImgs); // Quanti Descriptor Set allocare
	setAllocInfo.pSetLayouts		= setLayouts.data();							 // Layout da utilizzare per allocare i set (1:1)

	// Allocazione dei descriptor sets (molteplici)
	VkResult res = vkAllocateDescriptorSets(*m_Device, &setAllocInfo, m_DescriptorSets.data());

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Descriptor Sets!");
	}

	// Aggiornamento dei bindings dei descriptor sets
	for (size_t i = 0; i < numSwapChainImgs; ++i)
	{
		// VIEW-PROJECTION DESCRIPTOR
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = viewProjectionUBO[i];		// Buffer da cui prendere i dati
		vpBufferInfo.offset = 0;							// Offset da cui partire
		vpBufferInfo.range = dataSize;	// Dimensione dei dati

		VkWriteDescriptorSet vpSetWrite = {}; // DescriptorSet per m_uboViewProjection
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = m_DescriptorSets[i];
		vpSetWrite.dstBinding = 0;									// Vogliamo aggiornare quello che binda su 0
		vpSetWrite.dstArrayElement = 0;									// Se avessimo un array questo
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Indice nell'array da aggiornare
		vpSetWrite.descriptorCount = 1;									// Numero di descriptor da aggiornare
		vpSetWrite.pBufferInfo = &vpBufferInfo;						// Informazioni a riguardo dei dati del buffer da bindare
/*
		// MODEL DESCRIPTOR (DYNAMIC UBO)
		// Model Buffer Binding Info
		VkDescriptorBufferInfo modelBufferInfo = {};
		modelBufferInfo.buffer = m_modelDynamicUBO[i];
		modelBufferInfo.offset = 0;
		modelBufferInfo.range  = m_modelUniformAlignment;

		VkWriteDescriptorSet modelSetWrite = {};
		modelSetWrite.sType			  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		modelSetWrite.dstSet		  = m_descriptorSets[i];
		modelSetWrite.dstBinding	  = 1;										   // Vogliamo aggiornare quello che binda su 0
		modelSetWrite.dstArrayElement = 0;										   // Se avessimo un array questo
		modelSetWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // Indice nell'array da aggiornare
		modelSetWrite.descriptorCount = 1;										   // Numero di descriptor da aggiornare
		modelSetWrite.pBufferInfo	  = &modelBufferInfo;						   // Informazioni a riguardo dei dati del buffer da bindare
		*/
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

		// Aggiornamento dei descriptor sets con i nuovi dati del buffer/binding
		vkUpdateDescriptorSets(*m_Device, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

VkDescriptorSetLayout& DescriptorsHandler::GetViewProjectionDescriptorSetLayout()
{
	return m_ViewProjectionLayout;
}

VkDescriptorSetLayout& DescriptorsHandler::GetTextureDescriptorSetLayout()
{
	return m_TextureLayout;
}

VkDescriptorPool& DescriptorsHandler::GetVpPool()
{
	return m_ViewProjectionPool;
}

VkDescriptorPool& DescriptorsHandler::GetImguiDescriptorPool()
{
	return m_ImguiDescriptorPool;
}

VkDescriptorPool& DescriptorsHandler::GetTexturePool()
{
	return m_TexturePool;
}

std::vector<VkDescriptorSet>& DescriptorsHandler::GetDescriptorSets()
{
	return m_DescriptorSets;
}

void DescriptorsHandler::DestroyTexturePool()
{
	vkDestroyDescriptorPool(*m_Device, m_TexturePool, nullptr);
}

void DescriptorsHandler::DestroyTextureLayout()
{
	vkDestroyDescriptorSetLayout(*m_Device, m_TextureLayout, nullptr);
}

void DescriptorsHandler::DestroyViewProjectionPool()
{
	vkDestroyDescriptorPool(*m_Device, m_ViewProjectionPool, nullptr);
}

void DescriptorsHandler::DestroyViewProjectionLayout()
{
	vkDestroyDescriptorSetLayout(*m_Device, m_ViewProjectionLayout, nullptr);
}

void DescriptorsHandler::DestroyImguiDescriptorPool()
{
	vkDestroyDescriptorPool(*m_Device, m_ImguiDescriptorPool, nullptr);
}
