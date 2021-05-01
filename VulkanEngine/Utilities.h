#pragma once

#include "DataStructures.h"
#include <stb_image.h>
#ifdef NDEBUG										
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 3;
const int MAX_OBJECTS = 2;

class Utility
{
public:
	/* IMAGES */
	static VkImage CreateImage(VkPhysicalDevice& physicalDevice, VkDevice& device,
  							   uint32_t width, uint32_t height, VkFormat format, 
							   VkImageTiling tiling, VkImageUsageFlags useFlags, 
							   VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);

	static VkImageView CreateImageView(VkDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspetFlags);
	
	/* SHADERS */
	static VkShaderModule CreateShaderModule(VkDevice& device, const std::vector<char>& code);


	/* TEXTURE */
	static int CreateTexture(MainDevice& mainDevice, TextureObjects& textureObjects, 
							 VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool, std::string fileName);
	
	static int CreateTextureImage(MainDevice& mainDevice, TextureObjects& textureObjects,
								  VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool, std::string fileName);

	static stbi_uc* LoadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);

	static int CreateTextureDescriptor(VkDevice& device, VkImageView textureImage, TextureObjects& textureObjects);

private:
	Utility();
};

// Prototipi
uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t supportedMemoryTypes, VkMemoryPropertyFlags properties);

std::vector<char> ReadFile(const std::string& filename);

void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
	VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
	VkMemoryAllocateFlags bufferProperties,
	VkBuffer* buffer, VkDeviceMemory* bufferMemory);

VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool);
void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);

void copyBuffer(VkDevice logicalDevice, VkQueue transferQueue,
				VkCommandPool transferCommandPool, VkBuffer srcBuffer,
				VkBuffer dstBuffer, VkDeviceSize bufferSize);

void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer src, VkImage image, uint32_t width, uint32_t height);

void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);