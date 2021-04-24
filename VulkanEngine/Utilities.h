#pragma once

#include <fstream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

const int MAX_FRAMES_IN_FLIGHT = 3;
const int MAX_OBJECTS		   = 2;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 col;
};

// Indici delle Queue Families presenti nel device fisico
struct QueueFamilyIndices {

	int graphicsFamily	   = -1;		
	int presentationFamily = -1;		
									
	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

// Informazioni dell SwapChain
struct SwapChainDetails {
	// Capacità della Surface: numero di immagini, extent, ...
	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};	 
	
	// Lista di coppie <Formato Immagine, Spazio Colore> supportati nella SwapChain/Surface
	std::vector<VkSurfaceFormatKHR> formats;

	// Presentation Modes supportate dalla SwapChain/Surface
	std::vector<VkPresentModeKHR> presentationModes;	
};

// Struttura Image + ImageView
struct SwapChainImage {
	VkImage image;			
	VkImageView imageView;
};

// Prototipi
uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t supportedMemoryTypes, VkMemoryPropertyFlags properties);

std::vector<char> readFile(const std::string& filename);

void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
	VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
	VkMemoryAllocateFlags bufferProperties,
	VkBuffer* buffer, VkDeviceMemory* bufferMemory);

void copyBuffer(VkDevice logicalDevice, VkQueue transferQueue,
				VkCommandPool transferCommandPool, VkBuffer srcBuffer,
				VkBuffer dstBuffer, VkDeviceSize bufferSize);