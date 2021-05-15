#pragma once

#include "pch.h"

#include "Mesh.h"
#include <stb_image.h>

const int MAX_FRAMES_IN_FLIGHT = 3;
const int MAX_OBJECTS = 20;

class Utility
{
public:
	/* GENERIC */
	static std::vector<char> ReadFile(const std::string& filename);
	static VkFormat ChooseSupportedFormat(MainDevice& mainDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
	static void GetQueueFamilyIndices(VkPhysicalDevice &physicalDevice, VkSurfaceKHR& surface, QueueFamilyIndices& queueFamilyIndices);
	static bool CheckDeviceExtensionSupport(VkPhysicalDevice& device, const std::vector<const char*>& requestedDeviceExtensions);

	/* IMAGES */
	static VkImage CreateImage(MainDevice &mainDevice, uint32_t width, uint32_t height, VkFormat format, 
							   VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);

	static VkImageView CreateImageView(const VkDevice& logical_device, const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspect_flags);

	static void CreateDepthBufferImage(DepthBufferImage& image, MainDevice& mainDevice, VkExtent2D imgExtent);

	/* MEMORY */
	static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t supportedMemoryTypes, VkMemoryPropertyFlags properties);
	
	/* BUFFERS */
	static void CreateBuffer(const MainDevice& mainDevice, const BufferSettings &buffer_settings, VkBuffer *data, VkDeviceMemory *memory);
	static void CopyBuffer(VkDevice& logicalDevice, VkQueue& transferQueue, VkCommandPool& transferCommandPool, VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize bufferSize);
	static void CopyImageBuffer(VkDevice &device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer src, VkImage image, uint32_t width, uint32_t height);

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
	static VkCommandBuffer BeginCommandBuffer(VkDevice device, VkCommandPool commandPool);
	static void EndAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);

	/* SHADERS */
	static VkShaderModule CreateShaderModule(VkDevice& device, const std::vector<char>& code);

	/* TEXTURE */
	static int CreateTexture(
		MainDevice& mainDevice, VkDescriptorPool& texturePool, VkDescriptorSetLayout& textureLayout,
		TextureObjects& textureObjects, VkQueue& graphicsQueue,
		VkCommandPool& graphicsCommandPool,
		std::string fileName);
	static int CreateTextureImage(MainDevice& mainDevice, TextureObjects& textureObjects, VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool, std::string fileName);
	static stbi_uc* LoadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);
	static void TransitionImageLayout(VkDevice &device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
	
	static int CreateTextureDescriptor(VkDevice& device, VkImageView textureImage, VkDescriptorPool& texturePool, VkDescriptorSetLayout& textureLayout, TextureObjects& textureObjects);

private:
	Utility();
};