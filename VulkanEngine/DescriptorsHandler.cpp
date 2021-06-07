#include "pch.h"
#include "Utilities.h"
#include "DescriptorsHandler.h"

Descriptors::Descriptors()
{
	m_ViewProjectionLayout  = {};
	m_Device				= {};
	m_ViewProjectionPool	= {};
	m_ViewProjectionLayout	= {};
	m_TexturePool			= {};
	m_TextureLayout			= {};

	std::vector<VkDescriptorSet> m_DescriptorSets = {};
}

Descriptors::Descriptors(VkDevice *device)
{
	m_Device = device;
}

void Descriptors::CreateDescriptorPools(size_t swapchain_images, size_t vp_ubo_size, size_t light_ubo_size)
{
	CreateViewProjectionPool(swapchain_images, vp_ubo_size);
	CreateTexturePool();
	CreateImguiPool();
	CreateInputAttachmentsPool(swapchain_images);
	CreateLightPool(swapchain_images, light_ubo_size);
}

void Descriptors::CreateSetLayouts()
{
	CreateViewProjectionSetLayout();
	CreateTextureSetLayout();
	CreateInputSetLayout();
	CreateLightSetLayout();
}

void Descriptors::CreateViewProjectionSetLayout()
{
	VkDescriptorSetLayoutBinding viewProjectionLayoutBinding = {};
	viewProjectionLayoutBinding.binding				= 0;									
	viewProjectionLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
	viewProjectionLayoutBinding.descriptorCount		= 1;								
	viewProjectionLayoutBinding.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT;		
	viewProjectionLayoutBinding.pImmutableSamplers	= nullptr;						

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { viewProjectionLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount	= static_cast<uint32_t>(layoutBindings.size());
	layoutCreateInfo.pBindings		= layoutBindings.data();

	VkResult result = vkCreateDescriptorSetLayout(*m_Device, &layoutCreateInfo, nullptr, &m_ViewProjectionLayout);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create the View-Projection Descriptor Set Layout");
}

void Descriptors::CreateTextureSetLayout()
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
		throw std::runtime_error("Failed to create the Texture Descriptor Set Layout!");
	}
}

void Descriptors::CreateInputSetLayout()
{
	VkDescriptorSetLayoutBinding positionInputLayoutBinding = {};
	positionInputLayoutBinding.binding			= 0;
	positionInputLayoutBinding.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	positionInputLayoutBinding.descriptorCount	= 1;
	positionInputLayoutBinding.stageFlags		= VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding colourInputLayoutBinding = {};
	colourInputLayoutBinding.binding			= 1;
	colourInputLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	colourInputLayoutBinding.descriptorCount	= 1;
	colourInputLayoutBinding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding normalInputLayoutBinding = {};
	normalInputLayoutBinding.binding			= 2;
	normalInputLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalInputLayoutBinding.descriptorCount	= 1;
	normalInputLayoutBinding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayoutBinding> inputBindings = { positionInputLayoutBinding, colourInputLayoutBinding, normalInputLayoutBinding };

	VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
	inputLayoutCreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	inputLayoutCreateInfo.bindingCount	= static_cast<uint32_t>(inputBindings.size());
	inputLayoutCreateInfo.pBindings		= inputBindings.data();

	// Create Descriptor Set Layout
	VkResult result = vkCreateDescriptorSetLayout(*m_Device, &inputLayoutCreateInfo, nullptr, &m_InputLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create the Input Descriptor Set Layout!");
	}
}

void Descriptors::CreateLightSetLayout()
{
	VkDescriptorSetLayoutBinding light_layout_binding = {};
	light_layout_binding.binding			= 0;
	light_layout_binding.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	light_layout_binding.descriptorCount	= 1;
	light_layout_binding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;			
	light_layout_binding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layout_bindings = { light_layout_binding };

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount	= static_cast<uint32_t>(layout_bindings.size());
	layout_info.pBindings		= layout_bindings.data();

	VkResult result = vkCreateDescriptorSetLayout(*m_Device, &layout_info, nullptr, &m_LightLayout);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create the Light Descriptor Set Layout");
}

void Descriptors::CreateViewProjectionDescriptorSets(
	const std::vector<VkBuffer> & viewProjectionUBO,
	size_t dataSize, size_t swapchain_size)
{
	m_DescriptorSets.resize(swapchain_size);

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
		vpBufferInfo.buffer = viewProjectionUBO[i];		
		vpBufferInfo.offset = 0;						
		vpBufferInfo.range	= dataSize;

		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet			= m_DescriptorSets[i];
		vpSetWrite.dstBinding		= 0;									// Vogliamo aggiornare quello che binda su 0
		vpSetWrite.dstArrayElement	= 0;									// Se avessimo un array questo
		vpSetWrite.descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Indice nell'array da aggiornare
		vpSetWrite.descriptorCount	= 1;									// Numero di descriptor da aggiornare
		vpSetWrite.pBufferInfo		= &vpBufferInfo;						// Informazioni a riguardo dei dati del buffer da bindare

		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

		vkUpdateDescriptorSets(*m_Device, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

void Descriptors::CreateInputAttachmentsDescriptorSets(size_t swapchain_size, const std::vector<BufferImage>& position_buffer,
	const std::vector<BufferImage>& color_buffer, const std::vector<BufferImage>& normal_buffer)
{
	m_InputDescriptorSets.resize(swapchain_size);

	std::vector<VkDescriptorSetLayout> inputSetLayouts(swapchain_size, m_InputLayout);

	VkDescriptorSetAllocateInfo input_allocate_info = {};
	input_allocate_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	input_allocate_info.descriptorPool		= m_InputPool;
	input_allocate_info.descriptorSetCount	= static_cast<uint32_t>(swapchain_size);
	input_allocate_info.pSetLayouts			= inputSetLayouts.data();

	VkResult result = vkAllocateDescriptorSets(*m_Device, &input_allocate_info, m_InputDescriptorSets.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Input Attachment Descriptor Sets!");
	}

	for (size_t i = 0; i < swapchain_size; i++)
	{
		VkDescriptorImageInfo positionImageInfo = {};
		positionImageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		positionImageInfo.imageView		= position_buffer[i].ImageView;
		positionImageInfo.sampler		= position_buffer[i].Sampler;

		VkDescriptorImageInfo colourImageInfo = {};
		colourImageInfo.imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colourImageInfo.imageView		= color_buffer[i].ImageView;
		colourImageInfo.sampler			= color_buffer[i].Sampler;

		VkDescriptorImageInfo normalImageInfo = {};
		normalImageInfo.imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalImageInfo.imageView		= normal_buffer[i].ImageView;
		normalImageInfo.sampler			= normal_buffer[i].Sampler;

		VkWriteDescriptorSet positionWrite = {};
		positionWrite.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		positionWrite.dstSet			= m_InputDescriptorSets[i];
		positionWrite.dstBinding		= 0;
		positionWrite.dstArrayElement	= 0;
		positionWrite.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		positionWrite.descriptorCount	= 1;
		positionWrite.pImageInfo		= &positionImageInfo;

		VkWriteDescriptorSet colourWrite = {};
		colourWrite.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		colourWrite.dstSet				= m_InputDescriptorSets[i];
		colourWrite.dstBinding			= 1;
		colourWrite.dstArrayElement		= 0;
		colourWrite.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		colourWrite.descriptorCount		= 1;
		colourWrite.pImageInfo			= &colourImageInfo;

		VkWriteDescriptorSet normalWrite = {};
		normalWrite.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		normalWrite.dstSet				= m_InputDescriptorSets[i];
		normalWrite.dstBinding			= 2;
		normalWrite.dstArrayElement		= 0;
		normalWrite.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalWrite.descriptorCount		= 1;
		normalWrite.pImageInfo			= &normalImageInfo;

		std::vector<VkWriteDescriptorSet> setWrites = { positionWrite, colourWrite, normalWrite };

		vkUpdateDescriptorSets(*m_Device, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

void Descriptors::CreateLightDescriptorSets(const std::vector<VkBuffer>& ubo_light, size_t data_size, size_t swapchain_images)
{
	m_LightDescriptorSets.resize(swapchain_images);

	std::vector<VkDescriptorSetLayout> desc_set_layouts(swapchain_images, m_LightLayout);

	VkDescriptorSetAllocateInfo light_allocate_info = {};
	light_allocate_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	light_allocate_info.descriptorPool		= m_LightPool;
	light_allocate_info.descriptorSetCount	= static_cast<uint32_t>(swapchain_images);
	light_allocate_info.pSetLayouts			= desc_set_layouts.data();

	VkResult result = vkAllocateDescriptorSets(*m_Device, &light_allocate_info, m_LightDescriptorSets.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Light Attachment Descriptor Sets!");
	}

	for (size_t i = 0; i < swapchain_images; i++)
	{
		VkDescriptorBufferInfo light_buffer_info = {};
		light_buffer_info.buffer = ubo_light[i];		
		light_buffer_info.offset = 0;				
		light_buffer_info.range	 = data_size;

		VkWriteDescriptorSet light_set_write = {};
		light_set_write.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		light_set_write.dstSet			= m_LightDescriptorSets[i];
		light_set_write.dstBinding		= 0;									
		light_set_write.dstArrayElement	= 0;									
		light_set_write.descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		light_set_write.descriptorCount	= 1;									
		light_set_write.pBufferInfo		= &light_buffer_info;					

		std::vector<VkWriteDescriptorSet> set_writes = { light_set_write };

		vkUpdateDescriptorSets(*m_Device, static_cast<uint32_t>(set_writes.size()), set_writes.data(), 0, nullptr);
	}
}

VkDescriptorSetLayout& Descriptors::GetViewProjectionSetLayout()
{
	return m_ViewProjectionLayout;
}

VkDescriptorSetLayout& Descriptors::GetTextureSetLayout()
{
	return m_TextureLayout;
}

VkDescriptorSetLayout& Descriptors::GetInputSetLayout()
{
	return m_InputLayout;
}

VkDescriptorSetLayout& Descriptors::GetLightSetLayout()
{
	return m_LightLayout;
}

VkDescriptorPool& Descriptors::GetVpPool()
{
	return m_ViewProjectionPool;
}

VkDescriptorPool& Descriptors::GetImguiDescriptorPool()
{
	return m_ImguiDescriptorPool;
}

VkDescriptorPool& Descriptors::GetTexturePool()
{
	return m_TexturePool;
}

VkDescriptorPool& Descriptors::GetInputPool()
{
	return m_InputPool;
}

VkDescriptorPool& Descriptors::GetLightPool()
{
	return m_LightPool;
}

std::vector<VkDescriptorSet>& Descriptors::GetDescriptorSets()
{
	return m_DescriptorSets;
}

std::vector<VkDescriptorSet>& Descriptors::GetInputDescriptorSets()
{
	return m_InputDescriptorSets;
}

std::vector<VkDescriptorSet>& Descriptors::GetLightDescriptorSets()
{
	return m_LightDescriptorSets;
}

void Descriptors::DestroyTexturePool()
{
	vkDestroyDescriptorPool(*m_Device, m_TexturePool, nullptr);
}

void Descriptors::DestroyTextureLayout()
{
	vkDestroyDescriptorSetLayout(*m_Device, m_TextureLayout, nullptr);
}

void Descriptors::DestroyViewProjectionPool()
{
	vkDestroyDescriptorPool(*m_Device, m_ViewProjectionPool, nullptr);
}

void Descriptors::DestroyViewProjectionLayout()
{
	vkDestroyDescriptorSetLayout(*m_Device, m_ViewProjectionLayout, nullptr);
}

void Descriptors::DestroyInputAttachmentsLayout()
{
	vkDestroyDescriptorSetLayout(*m_Device, m_InputLayout, nullptr);
}

void Descriptors::DestroyLightLayout()
{
	vkDestroyDescriptorSetLayout(*m_Device, m_LightLayout, nullptr);
}

void Descriptors::CreateViewProjectionPool(size_t numOfSwapImgs, size_t UBOsize)
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

void Descriptors::CreateTexturePool()
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

void Descriptors::CreateImguiPool()
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

void Descriptors::CreateInputAttachmentsPool(size_t numOfSwapImgs)
{
	VkDescriptorPoolSize position_pool_size = {};
	position_pool_size.type				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	position_pool_size.descriptorCount	= static_cast<uint32_t>(numOfSwapImgs);

	VkDescriptorPoolSize color_pool_size = {};
	color_pool_size.type			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	color_pool_size.descriptorCount = static_cast<uint32_t>(numOfSwapImgs);

	VkDescriptorPoolSize normal_pool_size = {};
	normal_pool_size.type				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normal_pool_size.descriptorCount	= static_cast<uint32_t>(numOfSwapImgs);

	std::vector<VkDescriptorPoolSize> pool_sizes = { position_pool_size, color_pool_size, normal_pool_size };

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

void Descriptors::CreateLightPool(size_t swapchain_images, size_t ubo_size)
{
	VkDescriptorPoolSize light_pool_size = {};
	light_pool_size.type			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	light_pool_size.descriptorCount = static_cast<uint32_t> (ubo_size); 

	std::vector<VkDescriptorPoolSize> pool_sizes = { light_pool_size };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.maxSets		= static_cast<uint32_t>(swapchain_images);
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());											  
	pool_info.pPoolSizes	= pool_sizes.data();		

	VkResult result = vkCreateDescriptorPool(*m_Device, &pool_info, nullptr, &m_LightPool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create the Light Descriptor Pool!");
	}

}

void Descriptors::DestroyImguiPool()
{
	vkDestroyDescriptorPool(*m_Device, m_ImguiDescriptorPool, nullptr);
}

void Descriptors::DestroyInputPool()
{
	vkDestroyDescriptorPool(*m_Device, m_InputPool, nullptr);
}

void Descriptors::DestroyLightPool()
{
	vkDestroyDescriptorPool(*m_Device, m_LightPool, nullptr);
}
