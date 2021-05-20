#include "pch.h"
#include "TextureLoader.h"

TextureLoader* TextureLoader::s_Instance = nullptr;

int TextureLoader::CreateTexture(const std::string& file_name)
{
	m_FileName = file_name;

	int const texture_image_location = CreateTextureImage();
	const VkImageView image_view = Utility::CreateImageView(m_TextureObjects->TextureImages[texture_image_location], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	m_TextureObjects->TextureImageViews.push_back(image_view);

	int const descriptor_location = CreateTextureDescriptor(image_view);

	return descriptor_location;
}

int TextureLoader::CreateTextureImage()
{
	CreateTextureBuffer();

	VkDeviceMemory m_TextureImageMemory;

	VkImage texture_image = CreateImage(&m_TextureImageMemory);

	// Descrive la pipeline barrier che deve rispettare la queue
	TransitionImageLayout(texture_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		Utility::CopyImageBuffer(m_StagingBuffer, texture_image, m_Width, m_Height);
	TransitionImageLayout(texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_TextureObjects->TextureImages.push_back(texture_image);
	m_TextureObjects->TextureImageMemory.push_back(m_TextureImageMemory);

	vkDestroyBuffer(m_MainDevice.LogicalDevice, m_StagingBuffer, nullptr);
	vkFreeMemory(m_MainDevice.LogicalDevice, m_StagingBufferMemory, nullptr);

	return static_cast<int>(m_TextureObjects->TextureImages.size()) - 1;
}

stbi_uc* TextureLoader::LoadTextureFile(int* width, int* height, VkDeviceSize* image_size)
{
	int nChannels;

	std::string fileLoc = "Textures/" + m_FileName;
	stbi_uc* image = stbi_load(fileLoc.c_str(), width, height, &nChannels, STBI_rgb_alpha);

	if (!image)
	{
		throw std::runtime_error("Failed to load a Texture file! (" + m_FileName + ")");
	}

	*image_size = static_cast<uint64_t>((*width)) * static_cast<uint64_t>((*height)) * 4L;

	return image;

	return nullptr;
}

int TextureLoader::CreateTextureDescriptor(const VkImageView& texture_image)
{
	VkDescriptorSet descriptor_set;

	VkDescriptorSetAllocateInfo set_alloc_info = {};
	set_alloc_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_alloc_info.descriptorPool		= m_TexDescriptorPool;
	set_alloc_info.descriptorSetCount	= 1;
	set_alloc_info.pSetLayouts			= &m_TextureLayout;

	VkResult result = vkAllocateDescriptorSets(m_MainDevice.LogicalDevice, &set_alloc_info, &descriptor_set);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Texture Descriptor Set!");
	}

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView		= texture_image;
	imageInfo.sampler		= m_TextureObjects->TextureSampler;

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet			= descriptor_set;
	descriptorWrite.dstBinding		= 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo		= &imageInfo;

	vkUpdateDescriptorSets(m_MainDevice.LogicalDevice, 1, &descriptorWrite, 0, nullptr);

	m_TextureObjects->SamplerDescriptorSets.push_back(descriptor_set);

	return static_cast<int>(m_TextureObjects->SamplerDescriptorSets.size()) - 1;
}

TextureLoader* TextureLoader::GetInstance()
{
	if (s_Instance == 0)
		s_Instance = new TextureLoader();

	return s_Instance;
}

void TextureLoader::CreateTextureBuffer()
{
	stbi_uc* image_data = LoadTextureFile(&m_Width, &m_Height, &m_Image_size);

	m_BufferSettings.size		= m_Image_size;
	m_BufferSettings.usage		= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	m_BufferSettings.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	Utility::CreateBuffer(m_BufferSettings, &m_StagingBuffer, &m_StagingBufferMemory);

	// copy image data to staging buffer
	void* data;
	vkMapMemory(m_MainDevice.LogicalDevice, m_StagingBufferMemory, 0, m_Image_size, 0, &data);
	memcpy(data, image_data, static_cast<size_t>(m_Image_size));
	vkUnmapMemory(m_MainDevice.LogicalDevice, m_StagingBufferMemory);

	stbi_image_free(image_data);
}

VkImage TextureLoader::CreateImage(VkDeviceMemory *m_TextureImageMemory)
{
	// create image to hold final texture
	ImageInfo image_info = {};
	image_info.width		= m_Width;
	image_info.height		= m_Height;
	image_info.format		= VK_FORMAT_R8G8B8A8_UNORM;
	image_info.tiling		= VK_IMAGE_TILING_OPTIMAL;
	image_info.usage		= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.properties	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	return Utility::CreateImage(image_info, m_TextureImageMemory);
}

void TextureLoader::TransitionImageLayout(const VkImage& image, const VkImageLayout& old_layout, const VkImageLayout& new_layout)
{
	VkCommandBuffer command_buffer = Utility::BeginCommandBuffer();

	VkImageMemoryBarrier image_memory_barrier = {};
	image_memory_barrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.oldLayout							= old_layout;					// Layout da cui spostarsi
	image_memory_barrier.newLayout							= new_layout;					// Layout in cui spostarsi
	image_memory_barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;		// Queue family da cui spostarsi
	image_memory_barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;		// Queue family in cui spostarsi
	image_memory_barrier.image								= image;						// Immagine su cui wrappare la barriera
	image_memory_barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel		= 0;
	image_memory_barrier.subresourceRange.levelCount		= 1;
	image_memory_barrier.subresourceRange.baseArrayLayer	= 0;
	image_memory_barrier.subresourceRange.layerCount		= 1;

	bool const old_layout_undefined = old_layout == VK_IMAGE_LAYOUT_UNDEFINED;
	bool const new_layout_transfer_dst_optimal = new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	bool const old_layout_transfer_dst_optimal = old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	bool const new_layout_shader_read_only = new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkPipelineStageFlags src_stage = 0;	// Stage dal quale è possibile iniziare la transizione
	VkPipelineStageFlags dst_stage = 0;	// Stage nel quale la transizione deve essere già terminata

	// La transizione da oldLayout a newLayout inizierà a partire dallo stage definito in srcAccessMask
	// ma prima del termine dello stage definito in dstAccessMask.
	if (old_layout_undefined && new_layout_transfer_dst_optimal)
	{
		image_memory_barrier.srcAccessMask = 0;								// Qualsiasi stage iniziale
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	// copyBufferImage è una operazione di write

		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;	// qualsiasi momento dopo l'inizio della pipeline
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;		// Primache provi a fare una write nel transfer stage!
	}
	else if (old_layout_transfer_dst_optimal && new_layout_shader_read_only)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;			// al termine delle operazioni di scrittura del transfer stage
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;	// prima che provi a a leggere il fragment shader
	}

	const VkDependencyFlags dep_flags = 0;

	const uint32_t memory_barrier_count = 0;
	const uint32_t buffer_memory_barrier_count = 0;
	const uint32_t image_memory_barrier_count = 1;

	vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, dep_flags,
		memory_barrier_count, nullptr,						// no global memory barrier
		buffer_memory_barrier_count, nullptr,				// no buffer memory barrier
		image_memory_barrier_count, &image_memory_barrier
	);

	Utility::EndAndSubmitCommandBuffer(command_buffer);
}

void TextureLoader::Init(const VulkanRenderData& data, TextureObjects *objs)
{
	m_MainDevice		= data.main_device;
	m_GraphicsQueue		= data.graphic_queue;
	m_TexDescriptorPool	= data.texture_descriptor_pool;
	m_TextureLayout		= data.texture_descriptor_layout;
	m_CommandPool		= data.command_pool;

	m_TextureObjects	= objs;
}