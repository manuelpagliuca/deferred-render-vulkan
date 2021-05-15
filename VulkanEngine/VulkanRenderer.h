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
#include "Scene.h"
#include "GUI.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	int Init(void* newWindow);
	void UpdateModel(int modelID, glm::mat4 newModel);
	void Draw(ImDrawData * draw_data);
	void Cleanup();

	VulkanRenderData GetRenderData() {
		VulkanRenderData data = {};
		data.instance				= m_VulkanInstance;
		data.physicalDevice			= m_MainDevice.PhysicalDevice;
		data.device					= m_MainDevice.LogicalDevice;
		data.graphicQueueIndex		= m_QueueFamilyIndices.GraphicsFamily;
		data.graphicQueue			= m_GraphicsQueue;
		data.imguiDescriptorPool	= m_DescriptorsHandler.GetImguiDescriptorPool();
		data.minImageCount			= 3;	// setup correct practice
		data.imageCount				= 3;
		data.renderPass				= m_RenderPassHandler.GetRenderPass();
		data.commandPool			= m_CommandHandler.GetCommandPool();
		data.commandBuffers			= m_CommandHandler.GetCommandBuffers();

		return data;
	}

	std::vector<SubmissionSyncObjects>& GetSyncObjects() { return m_SyncObjects; }
	int GetCurrentFrameIdx() { return m_CurrentFrame; }

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
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	int	m_CurrentFrame   = 0;	    
	TextureObjects	  m_TextureObjects;

	QueueFamilyIndices m_QueueFamilyIndices;			
	VkQueue	m_GraphicsQueue;							
	VkQueue	m_PresentationQueue;						

	DepthBufferImage m_DepthBufferImage;

	std::vector<VkBuffer>		 m_viewProjectionUBO;					
	std::vector<VkDeviceMemory>  m_viewProjectionUniformBufferMemory;	
	//std::vector<VkBuffer>		 m_modelDynamicUBO;						
	//std::vector<VkDeviceMemory>  m_modelDynamicUniformBufferMemory;		
	VkPushConstantRange			 m_PushCostantRange;

	std::vector<SubmissionSyncObjects> m_SyncObjects;

	Scene m_Scene;

private:
	std::vector<Mesh> m_MeshList;
	UboViewProjection m_UBOViewProjection;

	/* Core Renderer Functions */
	void CreateRenderFoundations();
	void CreateInstance();													
	void RetrievePhysicalDevice();	
	void CreateLogicalDevice();	
	void CreateSurface();
	void CreateSynchronisation();

	/* Auxiliary function for creation */
	void LoadGlfwExtensions(std::vector<const char*>& instanceExtensions);

	/* Uniform Data */
	void CreatePushCostantRange();
	void CreateUniformBuffers();
	void UpdateUniformBuffers(uint32_t imageIndex);
	void SetViewProjectionData();

	/* Funzioni di controllo */
	void HandleMinimization();
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtension); // Controlla se le estensioni (scaricaete) che si vogliono utilizzare sono supportate da Vulkan.
	bool CheckDeviceSuitable(VkPhysicalDevice device);							  // Controllo se il dispositivo fisico è adatto allo scopo del programma (ha una QueueFamily di tipo Graphics)
	bool CheckValidationLayerSupport(std::vector<const char*>* validationLayers); // Controllo se le Validation Layer fornite sono supportate da Vulkan.
};