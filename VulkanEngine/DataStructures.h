#include "pch.h"

struct MainDevice {
	VkPhysicalDevice PhysicalDevice;
	VkDevice		 LogicalDevice;
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
	VkSemaphore ImageAvailable; // Avvisa quanto l'immagine è disponibile
	VkSemaphore RenderFinished; // Avvisa quando il rendering è terminato
	VkFence		InFlight;			  // Fence per il frame in esecuzione
};

// Indici delle Queue Families presenti nel device fisico
struct QueueFamilyIndices {

	int GraphicsFamily	   = -1;
	int PresentationFamily = -1;

	bool isValid()
	{
		return GraphicsFamily >= 0 && PresentationFamily >= 0;
	}
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 col;
	glm::vec2 tex;
};