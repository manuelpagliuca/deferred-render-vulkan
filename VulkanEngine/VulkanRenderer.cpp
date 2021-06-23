#include "pch.h"

#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
	m_VulkanInstance					= 0;
	m_Surface							= 0;
	m_PushCostantRange					= {};
	m_GraphicsQueue						= 0;
	m_PresentationQueue					= 0;
	m_MainDevice.LogicalDevice			= 0;
	m_MainDevice.PhysicalDevice			= 0;
	m_ViewProjectionData.projection		= glm::mat4(1.f);
	m_ViewProjectionData.view			= glm::mat4(1.f);
	m_MainDevice.MinUniformBufferOffset	= 0;
	
	m_RenderPassHandler			= RenderPassHandler(&m_MainDevice, &m_SwapChain);
	m_Descriptors				= Descriptors(&m_MainDevice.LogicalDevice);
	m_SwapChain					= SwapChain(&m_MainDevice, &m_Surface, m_Window, m_QueueFamilyIndices);
	m_GraphicPipeline			= GraphicPipeline(&m_MainDevice, &m_SwapChain, &m_RenderPassHandler);
	m_CommandHandler			= CommandHandler(&m_MainDevice, &m_GraphicPipeline, &m_RenderPassHandler);
	m_OffScreenCommandHandler	= CommandHandler(&m_MainDevice, &m_GraphicPipeline, &m_RenderPassHandler);
}

int VulkanRenderer::Init(void* t_window)
{
	if (!t_window)
	{
		std::cerr << "Unable to transfer GLFW window" << std::endl;
		return EXIT_FAILURE;
	}

	m_Window = static_cast<GLFWwindow*>(t_window);

	try
	{
		Utility::Setup(&m_MainDevice, &m_Surface, &m_CommandHandler.GetCommandPool(), &m_GraphicsQueue);

		CreateKernel();

		m_SwapChain.CreateSwapChain();

		m_RenderPassHandler.CreateOffScreenRenderPass();
		m_RenderPassHandler.CreateRenderPass();
			
		m_Descriptors.CreateSetLayouts();
		VkDescriptorSetLayout vp_set_layout		= m_Descriptors.GetViewProjectionSetLayout();
		VkDescriptorSetLayout tex_set_layout	= m_Descriptors.GetTextureSetLayout();
		VkDescriptorSetLayout inp_set_layout	= m_Descriptors.GetInputSetLayout();
		VkDescriptorSetLayout light_set_layout	= m_Descriptors.GetLightSetLayout();
		m_GraphicPipeline.SetDescriptorSetLayouts(vp_set_layout, tex_set_layout, inp_set_layout, light_set_layout);

		SetupPushCostantRange();
		m_GraphicPipeline.SetPushCostantRange(m_PushCostantRange);

		m_GraphicPipeline.CreateGraphicPipeline();

		m_PositionBufferImages.resize(m_SwapChain.SwapChainImagesSize());
		m_ColorBufferImages.resize(m_SwapChain.SwapChainImagesSize());
		m_NormalBufferImages.resize(m_SwapChain.SwapChainImagesSize());

		for (size_t i = 0; i < m_ColorBufferImages.size(); i++)
		{
			Utility::CreatePositionBufferImage(m_PositionBufferImages[i], m_SwapChain.GetExtent());
			Utility::CreatePositionBufferImage(m_ColorBufferImages[i], m_SwapChain.GetExtent());
			Utility::CreatePositionBufferImage(m_NormalBufferImages[i], m_SwapChain.GetExtent());
		}

		Utility::CreateDepthBufferImage(m_DepthBufferImage, m_SwapChain.GetExtent());

		m_SwapChain.SetRenderPass(m_RenderPassHandler.GetRenderPassReference());
		m_SwapChain.CreateFrameBuffers(m_DepthBufferImage.ImageView, m_ColorBufferImages);

		CreateOffScreenFrameBuffer();

		m_CommandHandler.CreateCommandPool(m_QueueFamilyIndices);
		m_CommandHandler.CreateCommandBuffers(m_SwapChain.FrameBuffersSize());
		m_OffScreenCommandHandler.CreateCommandPool(m_QueueFamilyIndices);
		m_OffScreenCommandHandler.CreateCommandBuffers(m_SwapChain.FrameBuffersSize());
		m_TextureObjects.CreateSampler(m_MainDevice);

		CreateUniformBuffers();

		m_Descriptors.CreateDescriptorPools(m_SwapChain.SwapChainImagesSize(), m_ViewProjectionUBO.size(), m_LightUBO.size());

		m_Descriptors.CreateViewProjectionDescriptorSets(m_ViewProjectionUBO, sizeof(ViewProjectionData), m_SwapChain.SwapChainImagesSize());
		m_Descriptors.CreateInputAttachmentsDescriptorSets(m_SwapChain.SwapChainImagesSize(), m_PositionBufferImages, m_ColorBufferImages, m_NormalBufferImages);
		m_Descriptors.CreateLightDescriptorSets(m_LightUBO, sizeof(LightData), m_SwapChain.SwapChainImagesSize());

		CreateSynchronizationObjects();
		SetUniformDataStructures();

		TextureLoader::GetInstance()->Init(GetRenderData(), &m_TextureObjects);
		m_Scene.PassRenderData(GetRenderData());
		m_Scene.LoadScene(m_MeshList, m_TextureObjects);

		CreateMeshModel("Models/Vivi_Final.obj");
		CreateMeshModel("Models/Vivi_Final.obj");
		CreateMeshModel("Models/Vivi_Final.obj");
	}
	catch (std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return 0;
}

void VulkanRenderer::CreateOffScreenFrameBuffer() {
	m_OffScreenFrameBuffer.resize(m_SwapChain.FrameBuffersSize());

	for (uint32_t i = 0; i < m_SwapChain.FrameBuffersSize(); ++i)
	{
		std::array<VkImageView, 4> attachments = {
			m_PositionBufferImages[i].ImageView,
			m_ColorBufferImages[i].ImageView,
			m_NormalBufferImages[i].ImageView,
			m_DepthBufferImage.ImageView
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass		= m_RenderPassHandler.GetOffScreenRenderPass();
		frameBufferCreateInfo.attachmentCount	= static_cast<uint32_t>(attachments.size());
		frameBufferCreateInfo.pAttachments		= attachments.data();
		frameBufferCreateInfo.width				= m_SwapChain.GetExtentWidth();
		frameBufferCreateInfo.height			= m_SwapChain.GetExtentHeight();
		frameBufferCreateInfo.layers			= 1;

		VkResult result = vkCreateFramebuffer(m_MainDevice.LogicalDevice, &frameBufferCreateInfo, nullptr, &m_OffScreenFrameBuffer[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create an offscreen Framebuffer!");
		}
	}
}

void VulkanRenderer::UpdateModel(int modelID, glm::mat4 newModel)
{
	if (modelID >= m_MeshList.size())
		return;
	m_MeshModelList[modelID].SetModel(newModel);
	//m_MeshList[modelID].setModel(newModel);
}

void VulkanRenderer::Draw(ImDrawData *draw_data)
{
	vkWaitForFences(m_MainDevice.LogicalDevice, 1, &m_SyncObjects[m_CurrentFrame].InFlight, VK_TRUE, std::numeric_limits<uint64_t>::max()); // Aspetto che l'ultimo draw venga effettuato

	vkResetFences(m_MainDevice.LogicalDevice, 1, &m_SyncObjects[m_CurrentFrame].InFlight); // InFlight messo ad UNSIGNALED
	
	uint32_t image_idx;
	VkResult result = vkAcquireNextImageKHR(
						m_MainDevice.LogicalDevice, m_SwapChain.GetSwapChain(),
						std::numeric_limits<uint64_t>::max(),
					    m_SyncObjects[m_CurrentFrame].ImageAvailable, VK_NULL_HANDLE, &image_idx);
	
	m_CommandHandler.RecordCommands(
		draw_data, image_idx, m_SwapChain.GetExtent(),
		m_SwapChain.GetFrameBuffers(),
		m_Descriptors.GetLightDescriptorSets(),
		m_Descriptors.GetInputDescriptorSets());

	m_OffScreenCommandHandler.RecordOffScreenCommands(
		draw_data, image_idx, m_SwapChain.GetExtent(), m_OffScreenFrameBuffer,
		m_MeshList, m_MeshModelList, m_TextureObjects, 
		m_Descriptors.GetDescriptorSets(),
		m_Descriptors.GetInputDescriptorSets(),
		m_PositionBufferImages, m_ColorBufferImages, m_NormalBufferImages, m_QueueFamilyIndices);
		
	UpdateUniformBuffersWithData(image_idx);

	// Stages dove aspettare che il semaforo sia SIGNALED (all'output del final color)
	VkPipelineStageFlags waitStages[] =
	{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount	= 1;														
	submitInfo.pWaitSemaphores		= &m_SyncObjects[m_CurrentFrame].ImageAvailable;			
	submitInfo.pWaitDstStageMask	= waitStages;												
	submitInfo.commandBufferCount	= 1;														
	submitInfo.pCommandBuffers		= &m_OffScreenCommandHandler.GetCommandBuffer(image_idx);  
	submitInfo.signalSemaphoreCount = 1;														
	submitInfo.pSignalSemaphores	= &m_SyncObjects[m_CurrentFrame].OffScreenAvailable;		

	result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit Command Buffer to Queue!");
	}

	// Submit light calculation pipeline
	submitInfo.pWaitSemaphores		= &m_SyncObjects[m_CurrentFrame].OffScreenAvailable;
	submitInfo.pCommandBuffers		= &m_CommandHandler.GetCommandBuffer(image_idx);
	submitInfo.pSignalSemaphores	= &m_SyncObjects[m_CurrentFrame].RenderFinished;

	result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_SyncObjects[m_CurrentFrame].InFlight);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit Command Buffer to Queue!");
	}

	// Presentazione dell'immagine a schermo
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType			   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;												
	presentInfo.pWaitSemaphores	   = &m_SyncObjects[m_CurrentFrame].RenderFinished; 
	presentInfo.swapchainCount	   = 1;												
	presentInfo.pSwapchains		   = m_SwapChain.GetSwapChainData();	 			
	presentInfo.pImageIndices	   = &image_idx;									

	result = vkQueuePresentKHR(m_PresentationQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		HandleMinimization();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to present the image!");
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::HandleMinimization()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_Window, &width, &height);

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_Window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_MainDevice.LogicalDevice);

	m_CommandHandler.FreeCommandBuffers();

	m_GraphicPipeline.DestroyPipeline();

	m_SwapChain.DestroyFrameBuffers();
	m_SwapChain.DestroySwapChainImageViews();
	m_SwapChain.DestroySwapChain();
	m_SwapChain.SetRecreationStatus(true);
	m_SwapChain.CreateSwapChain();
	m_SwapChain.SetRecreationStatus(false);

	m_RenderPassHandler.CreateRenderPass();

	m_GraphicPipeline.CreateGraphicPipeline();

	m_PositionBufferImages.resize(m_SwapChain.SwapChainImagesSize());
	m_ColorBufferImages.resize(m_SwapChain.SwapChainImagesSize());
	m_NormalBufferImages.resize(m_SwapChain.SwapChainImagesSize());

	for (size_t i = 0; i < m_ColorBufferImages.size(); i++)
	{
		Utility::CreatePositionBufferImage(m_PositionBufferImages[i], m_SwapChain.GetExtent());
		Utility::CreatePositionBufferImage(m_ColorBufferImages[i], m_SwapChain.GetExtent());
		Utility::CreatePositionBufferImage(m_NormalBufferImages[i], m_SwapChain.GetExtent());
	}

	Utility::CreateDepthBufferImage(m_DepthBufferImage, m_SwapChain.GetExtent());

	m_SwapChain.CreateFrameBuffers(m_DepthBufferImage.ImageView, m_ColorBufferImages);
	m_CommandHandler.CreateCommandBuffers(m_SwapChain.FrameBuffersSize());
}

void VulkanRenderer::CreateInstance()
{
	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef ENABLED_VALIDATION_LAYERS
	if (!CheckValidationLayerSupport(&validationLayers))
		throw std::runtime_error("VkInstance doesn't support the required validation layers");
#endif 

	VkApplicationInfo appInfo = {};

	appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO; 
	appInfo.pApplicationName	= "Vulkan Render Application";		  
	appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);			  
	appInfo.pEngineName			= "VULKAN RENDERER";				  
	appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);			  
	appInfo.apiVersion			= VK_API_VERSION_1_0;				  

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	
	createInfo.pApplicationInfo		= &appInfo;									

	std::vector <const char*> instanceExtensions = std::vector<const char*>(); 
	LoadGlfwExtensions(instanceExtensions);									   
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);		   

	if (!CheckInstanceExtensionSupport(&instanceExtensions))								
		throw std::runtime_error("VkInstance doesn't support the required extensions");		
	
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(instanceExtensions.size()); 
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();						   
	
#ifdef ENABLED_VALIDATION_LAYERS
	createInfo.enabledLayerCount	= static_cast<uint32_t>(validationLayers.size());   
	createInfo.ppEnabledLayerNames	= validationLayers.data();						 
#else
	createInfo.enabledLayerCount = 0;								
	createInfo.ppEnabledLayerNames = nullptr;						
#endif
	VkResult res = vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance);					   

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan instance");

#ifdef ENABLED_VALIDATION_LAYERS
	DebugMessanger::GetInstance()->SetupDebugMessenger(m_VulkanInstance);
#endif
}

void VulkanRenderer::CreateKernel()
{
	CreateInstance();
	CreateSurface();
	RetrievePhysicalDevice();
	CreateLogicalDevice();
}

void VulkanRenderer::LoadGlfwExtensions(std::vector<const char*>& instanceExtensions)
{
	uint32_t glfwExtensionCount = 0;	
	const char** glfwExtensions;		

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); 
	for (size_t i = 0; i < glfwExtensionCount; ++i)							 
		instanceExtensions.push_back(glfwExtensions[i]);
}

void VulkanRenderer::CreateMeshModel(const std::string& file)
{
	// Import model scene
	Assimp::Importer importer;

	// aiProcess_Triangulate tutti gli oggetti vengono rappresentati come triangoli
	// aiProcess_FlipUVs : inverte i texels in modo che possano funzionare con Vulkan
	// aiProcess_JoinIdenticalVertices : 
	const aiScene* scene = importer.ReadFile(file, aiProcess_Triangulate | 
		aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	
	if (!scene)
	{
		throw std::runtime_error("Failed to load model! (" + file + ")");
	}

	// Caricamento delle texture (materials della scena)
	std::vector<std::string> texture_names = MeshModel::LoadMaterials(scene);

	// Mapping degli ID texture con gli ID dei descirptor 
	std::vector<int> mat_to_tex(texture_names.size());

	for (size_t i = 0; i < texture_names.size(); i++)
	{
		if (texture_names[i].empty())
		{
			mat_to_tex[i] = 0;
		}
		else
		{
			mat_to_tex[i] = TextureLoader::GetInstance()->CreateTexture(texture_names[i]);
		}
	}

	std::vector<Mesh> model_meshes = MeshModel::LoadNode(m_MainDevice.PhysicalDevice, m_MainDevice.LogicalDevice,
		m_GraphicsQueue, m_CommandHandler.GetCommandPool(), scene->mRootNode, scene, mat_to_tex);

	MeshModel mesh_Model = MeshModel(model_meshes);
	m_MeshModelList.push_back(mesh_Model);
}

bool VulkanRenderer::CheckInstanceExtensionSupport(std::vector<const char*>* extensionsToCheck)
{
	uint32_t nAvailableExt = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &nAvailableExt, nullptr);
	
	std::vector<VkExtensionProperties> availableExt(nAvailableExt);						
	vkEnumerateInstanceExtensionProperties(nullptr, &nAvailableExt, availableExt.data());

	for (const auto& proposedExt : *extensionsToCheck)			
	{																
		bool hasExtension = false;
		for (const auto& extension : availableExt)
		{
			if (strcmp(proposedExt, extension.extensionName))
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
			return false;
	}

	return true;
}

bool VulkanRenderer::CheckValidationLayerSupport(std::vector<const char*>* validationLayers)
{
	uint32_t nAvailableLayers;
	vkEnumerateInstanceLayerProperties(&nAvailableLayers, nullptr);				 

	std::vector<VkLayerProperties> availableLayers(nAvailableLayers);				 
	vkEnumerateInstanceLayerProperties(&nAvailableLayers, availableLayers.data());

	for (const auto& layerName : *validationLayers)					
	{																
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void VulkanRenderer::RetrievePhysicalDevice()
{
	uint32_t deviceCount = 0;

	vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, nullptr);

	if (deviceCount == 0)										  
		throw std::runtime_error("Can't find GPU that support Vulkan Instance!");

	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, deviceList.data()); 

	for (const auto& device : deviceList)			
	{												
		if (CheckDeviceSuitable(device))
		{
			m_MainDevice.PhysicalDevice = device;
			break;
		}
	}

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(m_MainDevice.PhysicalDevice, &deviceProperties);

	m_MainDevice.MinUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;// serve per DYNAMIC UBO
}

bool VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice possibleDevice)
{
	 /*Al momento non ci interessano particolari caratteristiche della GPU

	// Informazioni generiche a riguardo del dispositivo
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);*/

/*	// Informazioni rispetto ai servizi che offre il dispositvo
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	// Preleva dal dispositivo fisico gli indici delle QueueFamily per la Grafica e la Presentazione
	Utility::GetPossibleQueueFamilyIndices(possibleDevice, m_QueueFamilyIndices);

	// Controlla che le estensioni richieste siano disponibili nel dispositivo fisico
	bool const extensionSupported = Utility::CheckPossibleDeviceExtensionSupport(possibleDevice, m_RequestedDeviceExtensions);


	bool swapChainValid	= false;

	// Se le estensioni richieste sono supportate (quindi Surface compresa), si procede con la SwapChain
	if (extensionSupported)
	{						
		SwapChainDetails swapChainDetails = m_SwapChain.GetSwapChainDetails(possibleDevice, m_Surface);
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}

	return m_QueueFamilyIndices.isValid() && extensionSupported && swapChainValid /*&& deviceFeatures.samplerAnisotropy*/;	// Il dispositivo è considerato adatto se :
																					// 1. Gli indici delle sue Queue Families sono validi
																					// 2. Se le estensioni richieste sono supportate
																					// 3. Se la Swap Chain è valida 
																					// 4. Supporta il sampler per l'anisotropy (basta controllare una volta che lo supporti la mia scheda video poi contrassegno l'esistenza nel createLogicalDevice)
}

void VulkanRenderer::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> queueFamilyIndices = { m_QueueFamilyIndices.GraphicsFamily , m_QueueFamilyIndices.PresentationFamily };

	// DEVICE QUEUE (Queue utilizzate nel device logico)
	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType			 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; // Tipo di struttura dati
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;	// Indice della QueueFamily da utilizzare su questo dispositivo
		queueCreateInfo.queueCount		 = 1;					// Numero di QueueFamily
		float const priority			 = 1.f;			// La priorità è un valore (o più) che serve a Vulkan per gestire molteplici Queue
		queueCreateInfo.pQueuePriorities = &priority;	// che lavorano in contemporanea. Andrebbe fornita una lista di priority, ma per il momento passiamo soltanto un valore.
	
		queueCreateInfos.push_back(queueCreateInfo);	// Carico la create info dentro un vettore
	}

	// LOGICAL DEVICE
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType					 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;			// Tipo di strutturadati da utilizzare.
	deviceCreateInfo.queueCreateInfoCount	 = static_cast<uint32_t>(queueCreateInfos.size());	// Numero di queue utilizzate nel device logico (corrispondono al numero di strutture "VkDeviceQueueCreateInfo").
	deviceCreateInfo.pQueueCreateInfos		 = queueCreateInfos.data();							// Puntatore alle createInfo delle Queue
	deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(m_RequestedDeviceExtensions.size());	// Numero di estensioni da utilizzare sul dispositivo logico.
	deviceCreateInfo.ppEnabledExtensionNames = m_RequestedDeviceExtensions.data();							// Puntatore ad un array che contiene le estensioni abilitate (SwapChain, ...).
	
	// Informazioni rispetto ai servizi che offre il dispositvo (GEFORCE 1070 STRIX supporto l'anisotropy)
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy	= VK_TRUE;

	deviceCreateInfo.pEnabledFeatures	= &deviceFeatures;					// Features del dispositivo fisico che verranno utilizzate nel device logico (al momento nessuna).


	
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	VkResult result	= vkCreateDevice(m_MainDevice.PhysicalDevice, &deviceCreateInfo, nullptr, &m_MainDevice.LogicalDevice);	// Creo il device logico

	if (result != VK_SUCCESS)	// Nel caso in cui il Dispositivo Logico non venga creato con successo alzo un eccezione a runtime.
		throw std::runtime_error("Failed to create Logical Device!");

	vkGetDeviceQueue(							// Salvo il riferimento della queue grafica del device logico
		m_MainDevice.LogicalDevice,				// nella variabile m_GraphicsQueue
		m_QueueFamilyIndices.GraphicsFamily, 
		0,
		&m_GraphicsQueue);
		
	vkGetDeviceQueue(							 // Salvo il riferimento alla Presentation Queue del device logico
		m_MainDevice.LogicalDevice,				 // nella variabile 'm_PresentationQueue'. Siccome è la medesima cosa della
		m_QueueFamilyIndices.PresentationFamily, // queue grafica, nel caso in cui sia presente una sola Queue nel device 
		0,										 // allora si avranno due riferimenti 'm_PresentationQueue' e 'm_GraphicsQueue' alla stessa queue
		&m_PresentationQueue);
}

void VulkanRenderer::CreateSurface()
{
	VkResult res = glfwCreateWindowSurface(m_VulkanInstance, m_Window, nullptr, &m_Surface);
																		
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create the surface!");
	}
}

const int VulkanRenderer::GetCurrentFrame() const
{
	return m_CurrentFrame;
}

void VulkanRenderer::CreateSynchronizationObjects()
{
	m_SyncObjects.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;	

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkResult offscreen_available_sem = vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphore_info, nullptr, &m_SyncObjects[i].OffScreenAvailable);
		VkResult image_available_sem	 = vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphore_info, nullptr, &m_SyncObjects[i].ImageAvailable);
		VkResult render_finished_sem	 = vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphore_info, nullptr, &m_SyncObjects[i].RenderFinished);
		VkResult in_flight_fence		 = vkCreateFence(m_MainDevice.LogicalDevice, &fence_info, nullptr, &m_SyncObjects[i].InFlight);

		if (offscreen_available_sem != VK_SUCCESS || 
			image_available_sem		!= VK_SUCCESS ||
			render_finished_sem		!= VK_SUCCESS ||
			in_flight_fence			!= VK_SUCCESS)
			throw std::runtime_error("Failed to create semaphores and/or Fence!");
	}
}

void VulkanRenderer::SetupPushCostantRange()
{
	m_PushCostantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // Stage dove finiranno le push costant
	m_PushCostantRange.offset	  = 0;
	m_PushCostantRange.size		  = sizeof(Model);
}

void VulkanRenderer::SetUniformDataStructures()
{
	// View-Projection
	float const aspectRatio					= static_cast<float>(m_SwapChain.GetExtentWidth()) / static_cast<float>(m_SwapChain.GetExtentHeight());
	m_ViewProjectionData.projection			= glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.f);
	m_ViewProjectionData.view				= glm::lookAt(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.0f, 0.f));
	m_ViewProjectionData.projection[1][1]	= m_ViewProjectionData.projection[1][1] * -1.0f;

	// Lights
	Light l1 = Light(1.0f, 0.0f, 0.0f, 1.0f);
	l1.SetLightPosition(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
	Light l2 = Light(0.0f, 1.0f, 0.0f, 1.0f);
	l2.SetLightPosition(glm::vec4(0.0f, 0.0f, -1.0f, 1.0f));
	Light l3 = Light(0.0f, 0.0f, 1.0f, 1.0f);
	l3.SetLightPosition(glm::vec4(0.0f, 0.0f, -1.0f, 1.0f));

	m_LightData[0] = l1.GetUBOData();
	m_LightData[1] = l2.GetUBOData();
	m_LightData[2] = l3.GetUBOData();
}

void VulkanRenderer::CreateUniformBuffers()
{
	BufferSettings buffer_settings;
	buffer_settings.size		= sizeof(m_ViewProjectionData);
	buffer_settings.usage		= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buffer_settings.properties	= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	// Un UBO per ogni immagine della Swap Chain
	m_ViewProjectionUBO.resize(m_SwapChain.SwapChainImagesSize());
	m_ViewProjectionUBOMemory.resize(m_SwapChain.SwapChainImagesSize());
	
	/*
	m_modelDynamicUBO.resize(m_swapChainImages.size());
	m_modelDynamicUniformBufferMemory.resize(m_swapChainImages.size());*/

	for (size_t i = 0; i < m_SwapChain.SwapChainImagesSize(); i++)
	{
		Utility::CreateBuffer(buffer_settings, &m_ViewProjectionUBO[i], &m_ViewProjectionUBOMemory[i]);
		/* DYNAMIC UBO
		createBuffer(m_mainDevice.physicalDevice, m_mainDevice.LogicalDevice,
			modelBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_modelDynamicUBO[i], &m_modelDynamicUniformBufferMemory[i]);*/
	}

	buffer_settings.size = sizeof(LightData);

	m_LightUBO.resize(m_SwapChain.SwapChainImagesSize());
	m_LightUBOMemory.resize(m_SwapChain.SwapChainImagesSize());

	for (size_t i = 0; i < m_SwapChain.SwapChainImagesSize(); ++i)
	{
		Utility::CreateBuffer(buffer_settings, &m_LightUBO[i], &m_LightUBOMemory[i]);
	}
}

void VulkanRenderer::UpdateUniformBuffersWithData(uint32_t imageIndex)
{
	void* vp_data;
	vkMapMemory(m_MainDevice.LogicalDevice, m_ViewProjectionUBOMemory[imageIndex], 0, 
		sizeof(ViewProjectionData), 0, &vp_data);
	memcpy(vp_data, &m_ViewProjectionData, sizeof(ViewProjectionData));
	vkUnmapMemory(m_MainDevice.LogicalDevice, m_ViewProjectionUBOMemory[imageIndex]);

	void* light_data;
	auto light_data_size = m_LightData.size() * sizeof(LightData);
	vkMapMemory(m_MainDevice.LogicalDevice, m_LightUBOMemory[imageIndex], 0, light_data_size, 0, &light_data);
	memcpy(light_data, m_LightData.data(), light_data_size);
	vkUnmapMemory(m_MainDevice.LogicalDevice, m_LightUBOMemory[imageIndex]);
}

void VulkanRenderer::Cleanup()
{
	// Aspetta finchè nessun azione sia eseguita sul device senza distruggere niente
	// Tutte le operazioni effettuate all'interno della draw() sono in asincrono.
	// Questo significa che all'uscita del loop della init(), le operazionio di drawing
	// e di presentazione potrebbero ancora essere in corso ed eliminare le risorse mentre esse sono in corso è una pessima idea
	// quindi è corretto aspettare che il dispositivo sia inattivo prima di eliminare gli oggetti.
	vkDeviceWaitIdle(m_MainDevice.LogicalDevice);

	for (size_t i = 0; i < m_MeshModelList.size(); i++)
	{
		m_MeshModelList[i].DestroyMeshModel();
	}

	GUI::GetInstance()->Destroy();

	m_Descriptors.DestroyImguiPool();

	m_Descriptors.DestroyTexturePool();
	m_Descriptors.DestroyTextureLayout();

	vkDestroySampler(m_MainDevice.LogicalDevice, m_TextureObjects.TextureSampler, nullptr);

	for (size_t i = 0; i < m_TextureObjects.TextureImages.size(); i++)
	{
		vkDestroyImageView(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImageViews[i], nullptr);
		vkDestroyImage(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImages[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImageMemory[i], nullptr);
	}

	m_Descriptors.DestroyInputPool();
	m_Descriptors.DestroyInputAttachmentsLayout();
	for (size_t i = 0; i < m_ColorBufferImages.size(); i++)
	{
		m_PositionBufferImages[i].DestroyAndFree(m_MainDevice);
		m_ColorBufferImages[i].DestroyAndFree(m_MainDevice);
		m_NormalBufferImages[i].DestroyAndFree(m_MainDevice);
	}

	m_DepthBufferImage.DestroyAndFree(m_MainDevice);
	
	m_Descriptors.DestroyViewProjectionPool();
	m_Descriptors.DestroyViewProjectionLayout();

	for (size_t i = 0; i < m_ViewProjectionUBO.size(); ++i)
	{
		vkDestroyBuffer(m_MainDevice.LogicalDevice, m_ViewProjectionUBO[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_ViewProjectionUBOMemory[i], nullptr);
	}

	m_Descriptors.DestroyLightPool();
	m_Descriptors.DestroyLightLayout();

	for (size_t i = 0; i < m_LightUBO.size(); ++i)
	{
		vkDestroyBuffer(m_MainDevice.LogicalDevice, m_LightUBO[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_LightUBOMemory[i], nullptr);
	}

	for (size_t i = 0; i < m_MeshList.size(); i++)
	{
		m_MeshList[i].destroyBuffers();
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SyncObjects[i].RenderFinished, nullptr);
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SyncObjects[i].ImageAvailable, nullptr);
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SyncObjects[i].OffScreenAvailable, nullptr);
		vkDestroyFence(m_MainDevice.LogicalDevice, m_SyncObjects[i].InFlight, nullptr);
	}

	m_OffScreenCommandHandler.DestroyCommandPool();
	m_CommandHandler.DestroyCommandPool();

	for (auto framebuffer : m_OffScreenFrameBuffer)
		vkDestroyFramebuffer(m_MainDevice.LogicalDevice, framebuffer, nullptr);
	m_SwapChain.DestroyFrameBuffers();

	m_GraphicPipeline.DestroyPipeline();
	m_RenderPassHandler.DestroyRenderPass();

	m_SwapChain.DestroySwapChainImageViews();
	m_SwapChain.DestroySwapChain();

	vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);	// Distrugge la Surface (GLFW si utilizza solo per settarla)

#ifdef ENABLED_VALIDATION_LAYERS
	DebugMessanger::GetInstance()->Clear();
#endif

	vkDestroyDevice(m_MainDevice.LogicalDevice, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);					
}

const VulkanRenderData VulkanRenderer::GetRenderData()
{
	VulkanRenderData data = {};
	data.main_device				= m_MainDevice;
	data.instance					= m_VulkanInstance;
	data.physical_device			= m_MainDevice.PhysicalDevice;
	data.device						= m_MainDevice.LogicalDevice;
	data.graphic_queue_index		= m_QueueFamilyIndices.GraphicsFamily;
	data.graphic_queue				= m_GraphicsQueue;
	data.imgui_descriptor_pool		= m_Descriptors.GetImguiDescriptorPool();
	data.min_image_count			= 3;	// setup correct practice
	data.image_count				= 3;	// setup correct practice
	data.render_pass				= m_RenderPassHandler.GetRenderPass();
	data.command_pool				= m_CommandHandler.GetCommandPool();
	data.command_buffers			= m_CommandHandler.GetCommandBuffers();
	data.texture_descriptor_layout	= m_Descriptors.GetTextureSetLayout();
	data.texture_descriptor_pool	= m_Descriptors.GetTexturePool();

	return data;
}

VulkanRenderer::~VulkanRenderer()
{
	Cleanup();
}