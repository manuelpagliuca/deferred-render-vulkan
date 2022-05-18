#pragma once

#include "pch.h"

#include "Window.h"
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

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "MeshModel.h"
#include "Light.h"

constexpr std::size_t NUM_LIGHTS = 20;

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	int Init(Window* window);
	void UpdateModel(int modelID, glm::mat4 newModel);
	void UpdateCameraPosition(const glm::mat4& view_matrix);
	void UpdateLightPosition(unsigned int lightID, const glm::vec3 &pos);
	void UpdateLightColour(unsigned int lightID, const glm::vec3 &col);
	void Draw(ImDrawData * draw_data);
	void Cleanup();

	const VulkanRenderData GetRenderData();
	SettingsData* GetUBOSettingsRef();

	int const GetCurrentFrame() const;

private:
	VkInstance			m_VulkanInstance;
	MainDevice			m_MainDevice;
	VkSurfaceKHR		m_Surface;	
	Window*				m_Window;
	SwapChain			m_SwapChain;
	RenderPassHandler	m_RenderPassHandler;
	GraphicPipeline		m_GraphicPipeline;
	CommandHandler		m_CommandHandler;
	CommandHandler		m_OffScreenCommandHandler;
	Descriptors			m_Descriptors;

	const std::vector<const char*> m_RequestedDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	int	m_CurrentFrame   = 0;	    
	TextureObjects	  m_TextureObjects;

	QueueFamilyIndices m_QueueFamilyIndices;			
	VkQueue	m_GraphicsQueue;							
	VkQueue	m_PresentationQueue;						

	std::vector<BufferImage> m_PositionBufferImages;
	std::vector<BufferImage> m_ColorBufferImages;
	std::vector<BufferImage> m_NormalBufferImages;
	BufferImage m_DepthBufferImage;

	std::vector<VkFramebuffer>	 m_OffScreenFrameBuffer;
	
	std::vector<VkBuffer>		 m_ViewProjectionUBO;
	std::vector<VkDeviceMemory>  m_ViewProjectionUBOMemory;
	ViewProjectionData			 m_VPData;

	std::vector<VkBuffer>		 m_LightUBO;
	std::vector<VkDeviceMemory>	 m_LightUBOMemory;
	std::array<LightData, NUM_LIGHTS>		 m_LightData;

	std::vector<VkBuffer>		 m_SettingsUBO;
	std::vector<VkDeviceMemory>	 m_SettingsUBOMemory;
	SettingsData				 m_SettingsData;

	VkPushConstantRange			 m_PushCostantRange;

	std::vector<SubmissionSyncObjects> m_SyncObjects;

private:
	Scene m_Scene;
	std::vector<Mesh> m_MeshList;
	std::vector<MeshModel> m_MeshModelList;

private:
	void CreateOffScreenFrameBuffer();

	/* Core Renderer Functions */
	void CreateKernel();
	void CreateInstance();													
	void RetrievePhysicalDevice();	
	void CreateLogicalDevice();	
	void CreateSurface();
	void CreateSynchronizationObjects();


	void CreateMeshModel(const std::string& file);

	/* Auxiliary function for creation */
	void LoadGlfwExtensions(std::vector<const char*>& instanceExtensions);

	/* Uniform Data */
	void SetupPushCostantRange();
	void CreateUniformBuffers();
	void UpdateUniformBuffersWithData(uint32_t imageIndex);
	void SetUniformDataStructures();
	void SetLightsDataStructures();

	/* Funzioni di controllo */
	void HandleMinimization();
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtension); // Controlla se le estensioni (scaricaete) che si vogliono utilizzare sono supportate da Vulkan.
	bool CheckDeviceSuitable(VkPhysicalDevice device);							  // Controllo se il dispositivo fisico è adatto allo scopo del programma (ha una QueueFamily di tipo Graphics)
	bool CheckValidationLayerSupport(std::vector<const char*>* validationLayers); // Controllo se le Validation Layer fornite sono supportate da Vulkan.
};