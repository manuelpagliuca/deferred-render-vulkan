#include "pch.h"
#include "Utilities.h"
#include "DescriptorsHandler.h"

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

void DescriptorsHandler::CreateDescriptorPools(size_t numOfSwapImgs, size_t UBOsize)
{
	CreateViewProjectionPool(numOfSwapImgs, UBOsize);
	CreateTexturePool();
	CreateImguiPool();
	CreateInputPool(numOfSwapImgs);
}

void DescriptorsHandler::CreateSetLayouts()
{
	CreateViewProjectionDescriptorSetLayout();
	CreateTextureDescriptorSetLayout();
	CreateInputDescriptorSetLayout();
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
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding			= 0;
	samplerLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount	= 1;
	samplerLayoutBinding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
	textureLayoutCreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutCreateInfo.bindingCount	= 1;
	textureLayoutCreateInfo.pBindings		= &samplerLayoutBinding;

	VkResult result = vkCreateDescriptorSetLayout(*m_Device, &textureLayoutCreateInfo, nullptr, &m_TextureLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Set Layout!");
	}
}

void DescriptorsHandler::CreateInputDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding colourInputLayoutBinding = {};
	colourInputLayoutBinding.binding			= 0;
	colourInputLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colourInputLayoutBinding.descriptorCount	= 1;
	colourInputLayoutBinding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
	depthInputLayoutBinding.binding				= 1;
	depthInputLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputLayoutBinding.descriptorCount		= 1;
	depthInputLayoutBinding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayoutBinding> inputBindings = { colourInputLayoutBinding, depthInputLayoutBinding };

	VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
	inputLayoutCreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	inputLayoutCreateInfo.bindingCount	= static_cast<uint32_t>(inputBindings.size());
	inputLayoutCreateInfo.pBindings		= inputBindings.data();

	// Create Descriptor Set Layout
	VkResult result = vkCreateDescriptorSetLayout(*m_Device, &inputLayoutCreateInfo, nullptr, &m_InputLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Input Descriptor Set Layout!");
	}
}

void DescriptorsHandler::CreateDescriptorSets(const std::vector<VkBuffer> & viewProjectionUBO,
	size_t dataSize, size_t swapchain_size,
	const std::vector<BufferImage> &color_buffer, const BufferImage &depth_buffer)
{
	m_DescriptorSets.resize(swapchain_size);

	// Lista di tutti i possibili layour che useremo dal set (?) non capito TODO
	std::vector<VkDescriptorSetLayout> setLayouts(swapchain_size, m_ViewProjectionLayout);

	VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool		= m_ViewProjectionPool;						// Pool per l'allocazione dei Descriptor Set
	allocate_info.descriptorSetCount	= static_cast<uint32_t>(swapchain_size);	// Quanti Descriptor Set allocare
	allocate_info.pSetLayouts			= setLayouts.data();						// Layout da utilizzare per allocare i set (1:1)

	VkResult res = vkAllocateDescriptorSets(*m_Device, &allocate_info, m_DescriptorSets.data());

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate the descriptor sets for the View and Projection matrices!");
	}

	for (size_t i = 0; i < swapchain_size; ++i)
	{
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = viewProjectionUBO[i];		// Buffer da cui prendere i dati
		vpBufferInfo.offset = 0;						// Offset da cui partire
		vpBufferInfo.range	= dataSize;

		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet			= m_DescriptorSets[i];
		vpSetWrite.dstBinding		= 0;									// Vogliamo aggiornare quello che binda su 0
		vpSetWrite.dstArrayElement	= 0;									// Se avessimo un array questo
		vpSetWrite.descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Indice nell'array da aggiornare
		vpSetWrite.descriptorCount	= 1;									// Numero di descriptor da aggiornare
		vpSetWrite.pBufferInfo		= &vpBufferInfo;						// Informazioni a riguardo dei dati del buffer da bindare
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

		vkUpdateDescriptorSets(*m_Device, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}

	// INPUT DESCRIPTOR SET CRATION (REFACTORING)

	// Resize array to hold descriptor set for each swap chain image
	m_InputDescriptorSets.resize(swapchain_size);

	// Fill array of layouts ready for set creation
	std::vector<VkDescriptorSetLayout> inputSetLayouts(swapchain_size, m_InputLayout);

	// Input Attachment Descriptor Set Allocation Info
	VkDescriptorSetAllocateInfo input_allocate_info = {};
	input_allocate_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	input_allocate_info.descriptorPool		= m_InputPool;
	input_allocate_info.descriptorSetCount	= static_cast<uint32_t>(swapchain_size);
	input_allocate_info.pSetLayouts			= inputSetLayouts.data();

	// Allocate Descriptor Sets
	VkResult result = vkAllocateDescriptorSets(*m_Device, &input_allocate_info, m_InputDescriptorSets.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Input Attachment Descriptor Sets!");
	}

	// Update each descriptor set with input attachment
	for (size_t i = 0; i < swapchain_size; i++)
	{
		// Colour Attachment Descriptor
		VkDescriptorImageInfo colourAttachmentDescriptor = {};
		colourAttachmentDescriptor.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colourAttachmentDescriptor.imageView	= color_buffer[i].ImageView;
		colourAttachmentDescriptor.sampler		= VK_NULL_HANDLE;

		// Colour Attachment Descriptor Write
		VkWriteDescriptorSet colourWrite = {};
		colourWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		colourWrite.dstSet			= m_InputDescriptorSets[i];
		colourWrite.dstBinding		= 0;
		colourWrite.dstArrayElement = 0;
		colourWrite.descriptorType	= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		colourWrite.descriptorCount = 1;
		colourWrite.pImageInfo		= &colourAttachmentDescriptor;

		// Depth Attachment Descriptor
		VkDescriptorImageInfo depthAttachmentDescriptor = {};
		depthAttachmentDescriptor.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachmentDescriptor.imageView		= depth_buffer.ImageView;
		depthAttachmentDescriptor.sampler		= VK_NULL_HANDLE;

		// Depth Attachment Descriptor Write
		VkWriteDescriptorSet depthWrite = {};
		depthWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		depthWrite.dstSet			= m_InputDescriptorSets[i];
		depthWrite.dstBinding		= 1;
		depthWrite.dstArrayElement	= 0;
		depthWrite.descriptorType	= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthWrite.descriptorCount	= 1;
		depthWrite.pImageInfo		= &depthAttachmentDescriptor;

		// List of input descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { colourWrite, depthWrite };

		// Update descriptor sets
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

VkDescriptorSetLayout& DescriptorsHandler::GetInputDescriptorSetLayout()
{
	return m_InputLayout;
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

VkDescriptorPool& DescriptorsHandler::GetInputPool()
{
	return m_InputPool;
}

std::vector<VkDescriptorSet>& DescriptorsHandler::GetDescriptorSets()
{
	return m_DescriptorSets;
}

std::vector<VkDescriptorSet>& DescriptorsHandler::GetInputDescriptorSets()
{
	return m_InputDescriptorSets;
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

void DescriptorsHandler::DestroyInputSetLayout()
{
	vkDestroyDescriptorSetLayout(*m_Device, m_InputLayout, nullptr);
}

void DescriptorsHandler::CreateViewProjectionPool(size_t numOfSwapImgs, size_t UBOsize)
{
	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.type				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;			   // Che tipo di descriptor contiene (non è un descriptor set ma sono quelli presenti negli shaders)
	vpPoolSize.descriptorCount	= static_cast<uint32_t> (UBOsize); // Quanti descriptor

	// MODEL (DYNAMIC UBO)
	/*VkDescriptorPoolSize modelPoolSize = {};
	modelPoolSize.type			  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelPoolSize.descriptorCount = static_cast<uint32_t>(m_modelDynamicUBO.size()); */

	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets			= static_cast<uint32_t>(numOfSwapImgs); // un descriptor set per ogni commandbuffer/swapimage
	poolCreateInfo.poolSizeCount	= static_cast<uint32_t>(descriptorPoolSizes.size());											  // Quante pool
	poolCreateInfo.pPoolSizes		= descriptorPoolSizes.data();		// Array di pool size

	VkResult result = vkCreateDescriptorPool(*m_Device, &poolCreateInfo, nullptr, &m_ViewProjectionPool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}
}

void DescriptorsHandler::CreateTexturePool()
{
	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = MAX_OBJECTS;

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets			= MAX_OBJECTS;
	samplerPoolCreateInfo.poolSizeCount		= 1;
	samplerPoolCreateInfo.pPoolSizes		= &samplerPoolSize;

	VkResult result = vkCreateDescriptorPool(*m_Device, &samplerPoolCreateInfo, nullptr, &m_TexturePool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}
}

void DescriptorsHandler::CreateImguiPool()
{
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
	imgui_pool.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	imgui_pool.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	imgui_pool.maxSets			= 1000 * IM_ARRAYSIZE(imguiPoolSize);
	imgui_pool.poolSizeCount	= (uint32_t)IM_ARRAYSIZE(imguiPoolSize);
	imgui_pool.pPoolSizes		= imguiPoolSize;

	VkResult result = vkCreateDescriptorPool(*m_Device, &imgui_pool, nullptr, &m_ImguiDescriptorPool);
	
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}
}

void DescriptorsHandler::CreateInputPool(size_t numOfSwapImgs)
{
	VkDescriptorPoolSize color_pool_size = {};
	color_pool_size.type			= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	color_pool_size.descriptorCount = static_cast<uint32_t>(numOfSwapImgs);

	VkDescriptorPoolSize depth_pool_size = {};
	depth_pool_size.type				= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depth_pool_size.descriptorCount		= static_cast<uint32_t>(numOfSwapImgs);

	std::vector<VkDescriptorPoolSize> pool_sizes = { color_pool_size, depth_pool_size };

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.maxSets		= numOfSwapImgs;
	pool_create_info.poolSizeCount	= static_cast<uint32_t>(pool_sizes.size());
	pool_create_info.pPoolSizes		= pool_sizes.data();

	VkResult result = vkCreateDescriptorPool(*m_Device, &pool_create_info, nullptr, &m_InputPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create the Input Descriptor Pool!");
	}
}

void DescriptorsHandler::DestroyImguiDescriptorPool()
{
	vkDestroyDescriptorPool(*m_Device, m_ImguiDescriptorPool, nullptr);
}

void DescriptorsHandler::DestroyInputDescriptorPool()
{
	vkDestroyDescriptorPool(*m_Device, m_InputPool, nullptr);
}
