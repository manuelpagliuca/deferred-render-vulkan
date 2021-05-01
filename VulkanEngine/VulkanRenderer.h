#pragma once

#include "pch.h"

#include "Mesh.h"
#include "Utilities.h"
#include "DebugMessanger.h"

#include "SwapChainHandler.h"

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
	GLFWwindow* m_Window = nullptr;
	int m_currentFrame   = 0;	    

	const std::vector<const char*> m_requestedDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME		// SwapChain
	};

private:
	VkInstance		 m_VulkanInstance;
	MainDevice		 m_MainDevice;
	VkSurfaceKHR	 m_Surface;	

	SwapChainHandler m_SwapChainHandler;

	TextureObjects	 m_TextureObjects;

	/* Struct indici delle Queue Families */
	/* Queues */
	QueueFamilyIndices m_queueFamilyIndices;			
	VkQueue	m_graphicsQueue;							
	VkQueue	m_presentationQueue;						


	VkImage			m_depthBufferImage;			// Non c'è necessità di un vettore, questa immagine non viene utilizzata dopo la pipeline, serve solo per un compito poi ritornerà disponibile per disegnarci
	VkFormat		m_depthFormat;
	VkDeviceMemory  m_depthBufferImageMemory;
	VkImageView		m_depthBufferImageView;


	/* Variabili ausiliarie */
		
	VkDeviceSize m_minUniformBufferOffset;
	size_t		 m_modelUniformAlignment;
	Model*	 m_modelTransferSpace;


	/* Pipeline */
	VkPipeline		 m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass	 m_renderPass;

	/* Command Pools */
	VkCommandPool m_graphicsComandPool;
	std::vector<VkCommandBuffer> m_commandBuffer;

	/* Descriptors */
	VkDescriptorSetLayout		 m_descriptorSetLayout;   // Descrive i dati prima di passarli a gli Shaders
	VkDescriptorPool			 m_descriptorPool;		  // Utilizzato per allocare i Descriptor Sets

	std::vector<VkDescriptorSet> m_descriptorSets;		// Vettore di Descriptor Set
	

	std::vector<VkBuffer>		 m_viewProjectionUBO;		  // Uniform Buffer per caricare i dati sulla GPU
	std::vector<VkDeviceMemory>  m_viewProjectionUniformBufferMemory; // Memoria effettivamente allocata sulla GPU per il Descriptor Set

	std::vector<VkBuffer>		 m_modelDynamicUBO;		// Uniform Buffer per caricare i dati sulla GPU
	std::vector<VkDeviceMemory>  m_modelDynamicUniformBufferMemory; // Memoria effettivamente allocata sulla GPU per il Descriptor Set

	VkPushConstantRange			 m_pushCostantRange;

	// Oggetti per la sincronizzazione
	std::vector<SubmissionSyncObjects> m_syncObjects;
public:
	// Scene Objects
	std::vector<Mesh> m_meshList;
	
	// Scene Settings
	struct UboViewProjection {
		glm::mat4 projection;
		glm::mat4 view;
	} m_UboViewProjection;

	// Funzioni per la creazione
	void CreateInstance();													// Creawebzione dell'istanza di Vulkan
	void loadGlfwExtensions(std::vector<const char*>& instanceExtensions);	// Carica le estensioni di GLFW sul vettore (non controlla se sono supportate)
	void getQueueFamiliesIndices(VkPhysicalDevice device);					// Imposta l'indice delle Queue Families presenti sul Physical Device

	// Funzioni per l'inizializzazione del Vulkan Renderer
	void GetPhysicalDevice();	
	void CreateLogicalDevice();	
	void CreateSurface();

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

	/* Funzioni di scelta */
	VkFormat chooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
};