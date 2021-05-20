#pragma once

#include "pch.h"

#include "Mesh.h"

int constexpr MAX_FRAMES_IN_FLIGHT = 3;
int constexpr MAX_OBJECTS = 20;

class Utility
{
public:
	/* GENERIC */
	static std::vector<char> ReadFile(const std::string& filename);
	static VkFormat ChooseSupportedFormat(const MainDevice& main_device, const std::vector<VkFormat>& formats, const VkImageTiling &tiling, const VkFormatFeatureFlags &feature_flags);
	static void GetQueueFamilyIndices(const VkPhysicalDevice &physical_device, const VkSurfaceKHR& surface, QueueFamilyIndices& queue_family_indices);
	static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& requested_device_extensions);

	/* IMAGES */
	static VkImage CreateImage(const MainDevice &main_device, const ImageInfo& image_info, VkDeviceMemory* image_memory);
	static VkImageView CreateImageView(const VkDevice& logical_device, const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspect_flags);
	static void CreateDepthBufferImage(DepthBufferImage& image, const MainDevice& main_device, const VkExtent2D &img_extent);

	/* MEMORY */
	static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t supportedMemoryTypes, VkMemoryPropertyFlags properties);
	
	/* BUFFERS */
	static void CreateBuffer(const MainDevice& main_device, const BufferSettings &buffer_settings, VkBuffer *data, VkDeviceMemory *memory);
	static void CopyBufferCmd(const VkDevice& logical_device, const VkQueue& transfer_queue, const VkCommandPool& transfer_command_pool, const VkBuffer& src_buffer, const VkBuffer& dst_buffer, const VkDeviceSize &buffer_size);
	static void CopyImageBuffer(const VkDevice& device, const VkQueue& transfer_queue, const VkCommandPool& transfer_command_pool, const VkBuffer& src, const VkImage& image, const uint32_t width, const uint32_t height);

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
	static VkCommandBuffer BeginCommandBuffer(const VkDevice& device, const VkCommandPool& commandPool);
	static void EndAndSubmitCommandBuffer(const VkDevice& device, const VkCommandPool& command_pool, const VkQueue& queue, const VkCommandBuffer& command_buffer);

	/* SHADERS */
	static VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& code);
private:
	Utility();
};