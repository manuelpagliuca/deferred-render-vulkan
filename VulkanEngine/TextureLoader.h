#pragma once

#include "Utilities.h"

class TextureLoader
{
public:
	void Init(const VulkanRenderData& data, TextureObjects* objs);
	int CreateTexture(const std::string& file_name);
	int CreateTextureImage();
	stbi_uc* LoadTextureFile(int* width, int* height, VkDeviceSize* imageSize);
	void TransitionImageLayout(const VkImage& image, const VkImageLayout& old_layout, const VkImageLayout& new_layout);
	int CreateTextureDescriptor(const VkImageView& texture_image);

	static TextureLoader* GetInstance();

private:
	MainDevice	m_MainDevice;
	VkQueue		m_GraphicsQueue;

	TextureObjects* m_TextureObjects;

	VkDescriptorPool		m_TexDescriptorPool;
	VkDescriptorSetLayout	m_TextureLayout;
	VkCommandPool			m_CommandPool;

private:
	TextureLoader() = default;
	static TextureLoader* s_Instance;

	std::string m_FileName;
	int m_Width;
	int m_Height;
	VkDeviceSize m_Image_size;
	VkBuffer		m_StagingBuffer;
	VkDeviceMemory	m_StagingBufferMemory;
	BufferSettings	m_BufferSettings;

private:
	void CreateTextureBuffer();
	VkImage CreateImage(VkDeviceMemory* m_TextureImageMemory);
};