#pragma once

#include "pch.h"

#include "Mesh.h"

int constexpr MAX_FRAMES_IN_FLIGHT	= 3;
int constexpr MAX_OBJECTS			= 20;

class Utility
{
public:
	static void Setup(MainDevice* main_device, VkSurfaceKHR* surface, VkCommandPool* command_pool, VkQueue* queue);

	/* GENERIC */
	static std::vector<char> ReadFile(const std::string& filename);
	static VkFormat ChooseSupportedFormat(const std::vector<VkFormat>& formats, const VkImageTiling& tiling, const VkFormatFeatureFlags& features);
	static void GetPossibleQueueFamilyIndices(const VkPhysicalDevice& potential_phys_dev, QueueFamilyIndices& family_indices);
	static bool CheckPossibleDeviceExtensionSupport(const VkPhysicalDevice& potential_phys_dev, const std::vector<const char*>& requested_dev_ext);

	/* IMAGES */
	static VkImage CreateImage(const ImageInfo& image_info, VkDeviceMemory* image_memory);
	static VkImageView CreateImageView(const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspect_flags);
	static VkSampler CreateSampler(const VkSamplerCreateInfo& sampler_create_info);
	static void CreateDepthBufferImage(BufferImage& image, const VkExtent2D &img_extent);
	static void CreatePositionBufferImage(BufferImage& image, const VkExtent2D& image_extent);
	static void CreateColorBufferImage(BufferImage& image, const VkExtent2D& img_extent);
	
	/* MEMORY */
	static uint32_t FindMemoryTypeIndex(uint32_t supportedMemoryTypes, const VkMemoryPropertyFlags& properties);
	
	/* BUFFERS */
	static void CreateBuffer(const BufferSettings& buffer_settings, VkBuffer* data, VkDeviceMemory* memory);
	static void CopyBufferCmd(const VkBuffer& src_buffer, const VkBuffer& dst_buffer, const VkDeviceSize& buffer_size);
	static void CopyImageBuffer(const VkBuffer& src, const VkImage& image, const uint32_t width, const uint32_t height);

	/* DYNAMIC UBO (SE NECESSARIO FARE UNA CLASSE PER I D-UBO)*/
	//static Model* AllocateDynamicBufferTransferSpace(VkDeviceSize minUniformBufferOffset);
	//static void FreeAlignedMemoryDUBO(Model* modelTransferSpace);
	// Distrugge i buffers per i DescriptorSet (Dynamic UBO)
	/*
	for (size_t i = 0; i < m_swapChainImages.size(); i++)
	{
		vkDestroyBuffer(m_mainDevice.LogicalDevice, m_viewProjectionUBO[i], nullptr);
		vkFreeMemory(m_mainDevice.LogicalDevice, m_viewProjectionUniformBufferMemory[i], nullptr);

		vkDestroyBuffer(m_mainDevice.LogicalDevice, m_modelDynamicUBO[i], nullptr);
		vkFreeMemory(m_mainDevice.LogicalDevice, m_modelDynamicUniformBufferMemory[i], nullptr);
	}*/

	/* COMMAND BUFFER */
	static VkCommandBuffer BeginCommandBuffer();
	static void EndAndSubmitCommandBuffer(const VkCommandBuffer& command_buffer);

	/* SHADERS */
	static VkShaderModule CreateShaderModule(const std::vector<char>& code);

private:
	Utility() = default;

	static MainDevice*		m_MainDevice;
	static VkSurfaceKHR*	m_Surface;
	static VkCommandPool*	m_CommandPool;
	static VkQueue*			m_GraphicsQueue;
};