#pragma once

#include "pch.h"

#include "Mesh.h"
#include "Utilities.h"
#include "DebugMessanger.h"

#include "SwapChainHandler.h"
#include "RenderPassHandler.h"
#include "GraphicPipeline.h"
#include "CommandHandler.h"
#include "DescriptorsHandler.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	int Init(void* newWindow);
	void UpdateModel(int modelID, glm::mat4 newModel);
	void Draw();
	void Cleanup();

private:
	VkInstance			m_VulkanInstance;
	MainDevice			m_MainDevice;
	VkSurfaceKHR		m_Surface;	
	GLFWwindow*			m_Window = nullptr;
	SwapChainHandler	m_SwapChainHandler;
	RenderPassHandler	m_RenderPassHandler;
	GraphicPipeline		m_GraphicPipeline;
	CommandHandler		m_CommandHandler;
	DescriptorsHandler	m_DescriptorsHandler;

	const std::vector<const char*> m_RequestedDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME		// SwapChain
	};

	int	m_CurrentFrame   = 0;	    
	TextureObjects	  m_TextureObjects;

	/* Struct indici delle Queue Families */
	/* Queues */
	QueueFamilyIndices m_QueueFamilyIndices;			
	VkQueue	m_graphicsQueue;							
	VkQueue	m_presentationQueue;						

	DepthBufferImage m_DepthBufferImage;

	std::vector<VkBuffer>		 m_viewProjectionUBO;					// Uniform Buffer per caricare i dati sulla GPU
	std::vector<VkDeviceMemory>  m_viewProjectionUniformBufferMemory;	// Memoria effettivamente allocata sulla GPU per il Descriptor Set

	std::vector<VkBuffer>		 m_modelDynamicUBO;						// Uniform Buffer per caricare i dati sulla GPU
	std::vector<VkDeviceMemory>  m_modelDynamicUniformBufferMemory;		// Memoria effettivamente allocata sulla GPU per il Descriptor Set

	VkPushConstantRange			 m_PushCostantRange;

	std::vector<SubmissionSyncObjects> m_SyncObjects;

private:
	std::vector<Mesh> m_MeshList;
	UboViewProjection m_UBOViewProjection;

	/* Core Renderer Functions */
	void CreateInstance();													
	void GetPhysicalDevice();	
	void CreateLogicalDevice();	
	void CreateSurface();
	void CreateSynchronisation();

	/* Uniform Data */
	void CreatePushCostantRange();
	void CreateUniformBuffers();
	void UpdateUniformBuffers(uint32_t imageIndex);

	/* Auxiliary function for creation*/
	void LoadGlfwExtensions(std::vector<const char*>& instanceExtensions);	// Carica le estensioni di GLFW sul vettore (non controlla se sono supportate)

	/* Funzioni di controllo */
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtension); // Controlla se le estensioni (scaricaete) che si vogliono utilizzare sono supportate da Vulkan.
	bool checkDeviceSuitable(VkPhysicalDevice device);							  // Controllo se il dispositivo fisico è adatto allo scopo del programma (ha una QueueFamily di tipo Graphics)
	bool CheckValidationLayerSupport(std::vector<const char*>* validationLayers); // Controllo se le Validation Layer fornite sono supportate da Vulkan.
};