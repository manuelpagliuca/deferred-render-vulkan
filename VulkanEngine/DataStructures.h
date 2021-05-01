#include "pch.h"

struct MainDevice {
	VkPhysicalDevice PhysicalDevice;
	VkDevice LogicalDevice;
};

struct TextureObjects {
	std::vector<VkImage>		 TextureImages;
	std::vector<VkDeviceMemory>  TextureImageMemory;
	std::vector<VkImageView>	 TextureImageViews;

	std::vector<VkDescriptorSet> SamplerDescriptorSets;
	VkDescriptorPool			 SamplerDescriptorPool;
	VkDescriptorSetLayout		 SamplerSetLayout;

	VkSampler					 TextureSampler;
};

struct SubmissionSyncObjects {
	VkSemaphore imageAvailable; // Avvisa quanto l'immagine è disponibile
	VkSemaphore renderFinished; // Avvisa quando il rendering è terminato
	VkFence inFlight;			  // Fence per il frame in esecuzione
};

// Indici delle Queue Families presenti nel device fisico
struct QueueFamilyIndices {

	int graphicsFamily = -1;
	int presentationFamily = -1;

	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 col;
	glm::vec2 tex;
};