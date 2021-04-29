#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>
#include <array>

#include "stb_image.h"

#include "Mesh.h"
#include "Utilities.h"

#ifdef NDEBUG										
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	int init(void* newWindow);

	void updateModel(int modelID, glm::mat4 newModel);

	void draw();
	void cleanup();

private:
	GLFWwindow* m_window = nullptr;
	int m_currentFrame   = 0;	    

	const std::vector<const char*> m_requestedDeviceExtensions =
	{
		// SwapChain
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
private:
	/* Istanza di Vulkan */
	VkInstance m_instance;

	/* Dispositivo Fisico & Logico */
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} m_mainDevice;										
	 
	/* Struct indici delle Queue Families */
	QueueFamilyIndices m_queueFamilyIndices;			

	/* Surface */
	VkSurfaceKHR m_surface;								// Surface di Vulkan (interfaccia per la swapchain)
	
	/* Swap Chain*/
	VkSwapchainKHR m_swapchain;							
	std::vector<SwapChainImage> m_swapChainImages;		 // Vettore contenente le Image e ImageView della SwapChain
	std::vector<VkFramebuffer>  m_swapChainFrameBuffers; //

	VkImage			m_depthBufferImage;			// Non c'è necessità di un vettore, questa immagine non viene utilizzata dopo la pipeline, serve solo per un compito poi ritornerà disponibile per disegnarci
	VkFormat		m_depthFormat;
	VkDeviceMemory  m_depthBufferImageMemory;
	VkImageView		m_depthBufferImageView;

	/* Queues */
	VkQueue	m_graphicsQueue;							
	VkQueue	m_presentationQueue;						

	/* Variabili ausiliarie */
	VkFormat	 m_swapChainImageFormat;   // Formato da utilizzare per l'Image View (prelevato dalla creazione della SwapChain)
	VkExtent2D	 m_swapChainExtent;		   // Extent da utilizzare per l'Image View (prelevato dalla creazione della SwapChain)
	
	VkDeviceSize m_minUniformBufferOffset;
	size_t		 m_modelUniformAlignment;
	Model*	 m_modelTransferSpace;

	VkSampler m_textureSampler;

	/* Assets */
	std::vector<VkImage> m_textureImages;
	std::vector<VkDeviceMemory> m_textureImageMemory;
	std::vector<VkImageView> m_textureImageViews;

	/* Pipeline */
	VkPipeline		 m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass	 m_renderPass;

	/* Command Pools */
	VkCommandPool m_graphicsComandPool;
	std::vector<VkCommandBuffer> m_commandBuffer;

	/* Descriptors */
	VkDescriptorSetLayout		 m_descriptorSetLayout;   // Descrive i dati prima di passarli a gli Shaders
	VkDescriptorSetLayout		 m_samplerSetLayout;
	VkDescriptorPool			 m_descriptorPool;		  // Utilizzato per allocare i Descriptor Sets
	VkDescriptorPool			 m_samplerDescriptorPool;

	std::vector<VkDescriptorSet> m_descriptorSets;		// Vettore di Descriptor Set
	std::vector<VkDescriptorSet> m_samplerDescriptorSets;

	std::vector<VkBuffer>		 m_viewProjectionUBO;		  // Uniform Buffer per caricare i dati sulla GPU
	std::vector<VkDeviceMemory>  m_viewProjectionUniformBufferMemory; // Memoria effettivamente allocata sulla GPU per il Descriptor Set

	std::vector<VkBuffer>		 m_modelDynamicUBO;		// Uniform Buffer per caricare i dati sulla GPU
	std::vector<VkDeviceMemory>  m_modelDynamicUniformBufferMemory; // Memoria effettivamente allocata sulla GPU per il Descriptor Set

	VkPushConstantRange			 m_pushCostantRange;

	/* Semafori */
	struct SubmissionSyncObjects {
		VkSemaphore m_imageAvailable; // Avvisa quanto l'immagine è disponibile
		VkSemaphore m_renderFinished; // Avvisa quando il rendering è terminato
		VkFence m_inFlight;	  // 
	};
	std::vector<SubmissionSyncObjects> m_syncObjects;

	/* Debug Messanger per Validation Layer*/
	VkDebugUtilsMessengerEXT m_debugMessenger;
private:

	// Scene Objects
	std::vector<Mesh> m_meshList;
	
	// Scene Settings
	struct UboViewProjection {
		glm::mat4 projection;
		glm::mat4 view;
	} m_UboViewProjection;

	// Funzioni per la creazione
	void createInstance();													// Creawebzione dell'istanza di Vulkan
	void loadGlfwExtensions(std::vector<const char*>& instanceExtensions);	// Carica le estensioni di GLFW sul vettore (non controlla se sono supportate)
	void getQueueFamiliesIndices(VkPhysicalDevice device);					// Imposta l'indice delle Queue Families presenti sul Physical Device
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	// Funzioni per l'inizializzazione del Vulkan Renderer
	void getPhysicalDevice();	
	void createLogicalDevice();	
	void createSurface();
	void createSwapChain();
	void createGraphicPipeline();
	void createRenderPass();
	void createDepthBufferImage();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createTextureSampler();
	void createSynchronisation();
	void createDescriptorSetLayout();
	void createPushCostantRange();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();

	void updateUniformBuffers(uint32_t imageIndex);

	/* Funzioni di registrazione */
	void recordCommands(uint32_t currentImage);

	/* Funzioni di allocazione */
	void allocateDynamicBufferTransferSpace();

	/* Funzioni di controllo */
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtension); // Controlla se le estensioni (scaricaete) che si vogliono utilizzare sono supportate da Vulkan.
	bool checkDeviceSuitable(VkPhysicalDevice device);							  // Controllo se il dispositivo fisico è adatto allo scopo del programma (ha una QueueFamily di tipo Graphics)
	bool checkValidationLayerSupport(std::vector<const char*>* validationLayers); // Controllo se le Validation Layer fornite sono supportate da Vulkan.
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);					  // Controllo se il dispositivo fisico supporta le estensioni specificate in 'deviceExtensions' (Utilities.h)

	/* Funzioni ausiliarie */
	VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory *imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspetFlags);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	int createTexture(std::string fileName);
	int createTextureImage(std::string fileName);
	int createTextureDescriptor(VkImageView textureImage);

	/* Funzioni di scelta */
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkFormat chooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	// Funzioni per il debug
	void setupDebugMessenger();										// Funzione che imposta il debug messanger
	
	/* Loader Functions */
	stbi_uc* loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(			// Funzione callback
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	VkResult CreateDebugUtilsMessengerEXT(						// Recupera la funzione per creare il debug messanger
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(							// Recupera la funzione per distruggere il debug messanger
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);
};