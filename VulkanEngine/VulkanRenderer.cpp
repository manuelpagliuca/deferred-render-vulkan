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
	m_UBOViewProjection.projection		= glm::mat4(1.f);
	m_UBOViewProjection.view			= glm::mat4(1.f);
	m_MainDevice.MinUniformBufferOffset	= 0;
	
	m_RenderPassHandler  = RenderPassHandler(&m_MainDevice, &m_SwapChainHandler);
	m_DescriptorsHandler = DescriptorsHandler(&m_MainDevice.LogicalDevice);
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
		CreateRenderKernel();

		m_SwapChainHandler	= SwapChainHandler(m_MainDevice, m_Surface, m_Window, m_QueueFamilyIndices);
		m_SwapChainHandler.CreateSwapChain();


		m_RenderPassHandler.CreateRenderPass();
		
		CreatePushCostantRange();
		
		m_DescriptorsHandler.CreateSetLayouts();
		VkDescriptorSetLayout vpDescSet	= m_DescriptorsHandler.GetViewProjectionDescriptorSetLayout();
		VkDescriptorSetLayout texDesSet	= m_DescriptorsHandler.GetTextureDescriptorSetLayout();

		m_GraphicPipeline	= GraphicPipeline(m_MainDevice, m_SwapChainHandler, &m_RenderPassHandler, vpDescSet, texDesSet, m_PushCostantRange);

		Utility::CreateDepthBufferImage(m_DepthBufferImage, m_MainDevice, m_SwapChainHandler.GetExtent());

		m_SwapChainHandler.SetRenderPass(m_RenderPassHandler.GetRenderPassReference());

		m_SwapChainHandler.CreateFrameBuffers(m_DepthBufferImage.ImageView);

		m_CommandHandler = CommandHandler(m_MainDevice, &m_GraphicPipeline, m_RenderPassHandler.GetRenderPassReference());
		m_CommandHandler.CreateCommandPool(m_QueueFamilyIndices);
		m_CommandHandler.CreateCommandBuffers(m_SwapChainHandler.FrameBuffersSize());
		m_TextureObjects.CreateTextureSampler(m_MainDevice);

		CreateUniformBuffers();

		m_DescriptorsHandler.CreateDescriptorPools(m_SwapChainHandler.SwapChainImagesSize(), m_viewProjectionUBO.size());
		m_DescriptorsHandler.CreateDescriptorSets(m_viewProjectionUBO, sizeof(UboViewProjection), m_SwapChainHandler.SwapChainImagesSize());

		CreateSynchronisation();
		SetViewProjectionData();

		m_Scene.PassRenderData(GetRenderData(), &m_DescriptorsHandler);
		m_Scene.LoadScene(m_MeshList, m_TextureObjects);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return 0;
}

void VulkanRenderer::UpdateModel(int modelID, glm::mat4 newModel)
{
	if (modelID >= m_MeshList.size())
		return;
	m_MeshList[modelID].setModel(newModel);
}

void VulkanRenderer::Draw(ImDrawData *draw_data)
{
	// Aspetta 1 Fence di essere nello stato di SIGNALED 
	// Aspettare per il dato Fence il segnale dell'ultimo draw effettuato prima di continuare
	vkWaitForFences(m_MainDevice.LogicalDevice, 1, &m_SyncObjects[m_CurrentFrame].InFlight, VK_TRUE, UINT64_MAX);

	// Metto la Fence ad UNSIGNALED (la GPU deve aspettare per la prossima operazione di draw)
	vkResetFences(m_MainDevice.LogicalDevice, 1, &m_SyncObjects[m_CurrentFrame].InFlight);
	
	// Recupero l'index della prossima immagine disponibile della SwapChain,
	// e mette SIGNALED il relativo semaforo 'm_imageAvailable' per avvisare
	// che l'immagine è pronta ad essere utilizzata.
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_MainDevice.LogicalDevice, m_SwapChainHandler.GetSwapChain(),
						  std::numeric_limits<uint64_t>::max(),
					      m_SyncObjects[m_CurrentFrame].ImageAvailable, VK_NULL_HANDLE, &imageIndex);

	
	m_CommandHandler.RecordCommands(draw_data, imageIndex, m_SwapChainHandler.GetExtent(),
		m_SwapChainHandler.GetFrameBuffers(), m_MeshList, m_TextureObjects, m_DescriptorsHandler.GetDescriptorSets());
	

	// Copia nell'UniformBuffer della GPU le matrici m_uboViewProjection
	UpdateUniformBuffers(imageIndex);

	// Stages dove aspettare che il semaforo sia SIGNALED (all'output del final color)
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	
	};
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	submitInfo.waitSemaphoreCount	= 1;											  // Numero dei semafori da aspettare
	submitInfo.pWaitSemaphores		= &m_SyncObjects[m_CurrentFrame].ImageAvailable;  // Semaforo di disponibilità dell'immagine (aspetta che sia SIGNALED)
	submitInfo.pWaitDstStageMask	= waitStages;									  // Stage dove iniziare la semaphore wait (output del final color, termine della pipeline)
	submitInfo.commandBufferCount	= 1;											  // Numero di Command Buffer da inviare alla Queue
	submitInfo.pCommandBuffers		= &m_CommandHandler.GetCommandBuffer(imageIndex); // Il Command Buffer da inviare non è quello corrente, ma è quello dell'immagine disponile
	submitInfo.signalSemaphoreCount = 1;											  // Numero di semafori a cui segnalare na volta che il CommandBuffer ha terminato
	submitInfo.pSignalSemaphores	= &m_SyncObjects[m_CurrentFrame].RenderFinished;  // Semaforo di fine Rendering (verrà messo a SIGNALED)


	// L'operazione di submit del CommandBuffer alla Queue accade se prima del termine della Pipeline
	// è presente una nuova immagine disponibile, cosa garantita trammite una semaphore wait su 'm_imageAvailable' 
	// posizionata proprio prima dell'output stage della Pipeline.
	// Una volta che l'immagine sarà disponibile verranno eseguite le operazioni del CommandBuffer 
	// sulla nuova immagine disponibile. Al termine delle operazioni del CommandBuffer il nuovo render sarà pronto
	// e verrà avvisato con il semaforo 'm_renderFinished'.
	// Inoltre al termine del render verrà anche avvisato il Fence, per dire che è possibile effettuare
	// l'operazione di drawing a schermo
	result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_SyncObjects[m_CurrentFrame].InFlight);
	
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit Command Buffer to Queue!");
	}

	// Presentazione dell'immagine a schermo
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType			   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;									// Numero di semafori da aspettare prima di renderizzare
	presentInfo.pWaitSemaphores	   = &m_SyncObjects[m_CurrentFrame].RenderFinished; // Aspetta che il rendering dell'immagine sia terminato
	presentInfo.swapchainCount	   = 1;									// Numero di swapchains a cui presentare
	presentInfo.pSwapchains		   = m_SwapChainHandler.GetSwapChainData();	 					// Swapchain contenente le immagini
	presentInfo.pImageIndices	   = &imageIndex;						// Indice dell'immagine nella SwapChain da visualizzare (quella nuova pescata dalle immagini disponibili)

	// Prima di effettuare l'operazione di presentazione dell'immagine a schermo si deve assicurare
	// che il render della nuova immagine sia pronto, cosa che viene assicurata dal semaforo 'm_renderFinished' del frame corrente.
	// Allora visto che l'immagine è ufficialmente pronta verrà mostrata a schermo
	result = vkQueuePresentKHR(m_PresentationQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		HandleMinimization();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to present the image!");

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

	m_SwapChainHandler.DestroyFrameBuffers();
	m_SwapChainHandler.DestroySwapChainImageViews();
	m_SwapChainHandler.DestroySwapChain();
	m_SwapChainHandler.SetRecreationStatus(true);
	m_SwapChainHandler.CreateSwapChain();
	m_SwapChainHandler.SetRecreationStatus(false);

	m_RenderPassHandler.CreateRenderPass();

	m_GraphicPipeline.CreateGraphicPipeline();

	Utility::CreateDepthBufferImage(m_DepthBufferImage, m_MainDevice, m_SwapChainHandler.GetExtent());

	m_SwapChainHandler.CreateFrameBuffers(m_DepthBufferImage.ImageView);

	m_CommandHandler.CreateCommandBuffers(m_SwapChainHandler.FrameBuffersSize());
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

void VulkanRenderer::CreateRenderKernel()
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
	Utility::GetQueueFamilyIndices(possibleDevice, m_Surface, m_QueueFamilyIndices);

	// Controlla che le estensioni richieste siano disponibili nel dispositivo fisico
	bool const extensionSupported = Utility::CheckDeviceExtensionSupport(possibleDevice, m_RequestedDeviceExtensions);


	bool swapChainValid	= false;

	// Se le estensioni richieste sono supportate (quindi Surface compresa), si procede con la SwapChain
	if (extensionSupported)
	{						
		SwapChainDetails swapChainDetails = m_SwapChainHandler.GetSwapChainDetails(possibleDevice, m_Surface);
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

void VulkanRenderer::CreateSynchronisation()
{
	m_SyncObjects.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;	

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkResult const imageAvailableSemaphore = vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphoreCreateInfo, nullptr, &m_SyncObjects[i].ImageAvailable);
		VkResult const renderFinishedSemaphore = vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphoreCreateInfo, nullptr, &m_SyncObjects[i].RenderFinished);
		VkResult const inFlightFence		   = vkCreateFence(m_MainDevice.LogicalDevice, &fenceCreateInfo, nullptr, &m_SyncObjects[i].InFlight);

		if (imageAvailableSemaphore != VK_SUCCESS || renderFinishedSemaphore != VK_SUCCESS || inFlightFence != VK_SUCCESS)
			throw std::runtime_error("Failed to create semaphores and/or Fence!");
	}
}

void VulkanRenderer::CreatePushCostantRange()
{
	m_PushCostantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // Stage dove finiranno le push costant
	m_PushCostantRange.offset	  = 0;
	m_PushCostantRange.size		  = sizeof(Model);
}

void VulkanRenderer::SetViewProjectionData()
{
	float const aspectRatio					= static_cast<float>(m_SwapChainHandler.GetExtentWidth()) / static_cast<float>(m_SwapChainHandler.GetExtentHeight());
	m_UBOViewProjection.projection			= glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.f);
	m_UBOViewProjection.view				= glm::lookAt(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.0f, 0.f));
	m_UBOViewProjection.projection[1][1]	= m_UBOViewProjection.projection[1][1] * -1.0f;
}

void VulkanRenderer::CreateUniformBuffers()
{
	BufferSettings buffer_settings;
	buffer_settings.size		= sizeof(m_UBOViewProjection);
	buffer_settings.usage		= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buffer_settings.properties	= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	// Un UBO per ogni immagine della Swap Chain
	m_viewProjectionUBO.resize(m_SwapChainHandler.SwapChainImagesSize());
	m_viewProjectionUniformBufferMemory.resize(m_SwapChainHandler.SwapChainImagesSize());
	
	/*
	m_modelDynamicUBO.resize(m_swapChainImages.size());
	m_modelDynamicUniformBufferMemory.resize(m_swapChainImages.size());*/

	for (size_t i = 0; i < m_SwapChainHandler.SwapChainImagesSize(); i++)
	{
		Utility::CreateBuffer(m_MainDevice, buffer_settings, &m_viewProjectionUBO[i], &m_viewProjectionUniformBufferMemory[i]);
		/* DYNAMIC UBO
		createBuffer(m_mainDevice.physicalDevice, m_mainDevice.LogicalDevice,
			modelBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_modelDynamicUBO[i], &m_modelDynamicUniformBufferMemory[i]);*/
	}
}

// Copia dei dati m_uboViewProjection della CPU nell'UniformBufferObject della GPU
void VulkanRenderer::UpdateUniformBuffers(uint32_t imageIndex)
{
	/* Copia dei dati CPU-GPU VIEW-PROJECTION */
	// Mappa l'UniformBufferObject nello spazio di indirizzamento dell'applicazione, salvando nel puntatore 'data' l'intervallo mappato sulla memoria.
	void* data;
	vkMapMemory(m_MainDevice.LogicalDevice, m_viewProjectionUniformBufferMemory[imageIndex], 0, 
		sizeof(UboViewProjection), 0, &data);
	memcpy(data, &m_UBOViewProjection, sizeof(UboViewProjection));
	vkUnmapMemory(m_MainDevice.LogicalDevice, m_viewProjectionUniformBufferMemory[imageIndex]);

	/* Copia delle Model matrix nei blocchi di memoria allineati (DYNAMIC UNIFORM BUFFER) */
	/*for(size_t i = 0; i < m_meshList.size(); i++)
	{
		Model* thisModel = (Model*)((uint64_t)m_modelTransferSpace + (i * m_modelUniformAlignment));
		*thisModel = m_meshList[i].getModel();
	}*/

	// Mappa l'UniformBufferObject nello spazio di indirizzamento dell'applicazione, salvando nel puntatore 'data' l'intervallo mappato sulla memoria.
	/*vkMapMemory(m_mainDevice.LogicalDevice,
		m_modelDynamicUniformBufferMemory[imageIndex], 0,
		m_modelUniformAlignment * m_meshList.size(), 0, &data);
	memcpy(data, m_modelTransferSpace, m_modelUniformAlignment * m_meshList.size());
	vkUnmapMemory(m_mainDevice.LogicalDevice, m_modelDynamicUniformBufferMemory[imageIndex]);*/
}

VulkanRenderer::~VulkanRenderer()
{
	Cleanup();
}

void VulkanRenderer::Cleanup()
{
	// Aspetta finchè nessun azione sia eseguita sul device senza distruggere niente
	// Tutte le operazioni effettuate all'interno della draw() sono in asincrono.
	// Questo significa che all'uscita del loop della init(), le operazionio di drawing
	// e di presentazione potrebbero ancora essere in corso ed eliminare le risorse mentre esse sono in corso è una pessima idea
	// quindi è corretto aspettare che il dispositivo sia inattivo prima di eliminare gli oggetti.
	vkDeviceWaitIdle(m_MainDevice.LogicalDevice);

	GUI::GetInstance()->Destroy();

	m_DescriptorsHandler.DestroyImguiDescriptorPool();

	m_DescriptorsHandler.DestroyTexturePool();
	m_DescriptorsHandler.DestroyTextureLayout();

	vkDestroySampler(m_MainDevice.LogicalDevice, m_TextureObjects.TextureSampler, nullptr);

	for (size_t i = 0; i < m_TextureObjects.TextureImages.size(); i++)
	{
		vkDestroyImageView(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImageViews[i], nullptr);
		vkDestroyImage(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImages[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImageMemory[i], nullptr);
	}

	m_DepthBufferImage.DestroyAndFree(m_MainDevice);

	m_DescriptorsHandler.DestroyViewProjectionPool();
	m_DescriptorsHandler.DestroyViewProjectionLayout();

	for (size_t i = 0; i < m_viewProjectionUBO.size(); ++i)
	{
		vkDestroyBuffer(m_MainDevice.LogicalDevice, m_viewProjectionUBO[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_viewProjectionUniformBufferMemory[i], nullptr);
	}

	for (size_t i = 0; i < m_MeshList.size(); i++)
		m_MeshList[i].destroyBuffers();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SyncObjects[i].RenderFinished, nullptr);
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SyncObjects[i].ImageAvailable, nullptr);
		vkDestroyFence(m_MainDevice.LogicalDevice, m_SyncObjects[i].InFlight, nullptr);
	}

	m_CommandHandler.DestroyCommandPool();
	m_SwapChainHandler.DestroyFrameBuffers();

	m_GraphicPipeline.DestroyPipeline();
	m_RenderPassHandler.DestroyRenderPass();

	m_SwapChainHandler.DestroySwapChainImageViews();
	m_SwapChainHandler.DestroySwapChain();

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
	data.main_device			= m_MainDevice;
	data.instance				= m_VulkanInstance;
	data.physical_device		= m_MainDevice.PhysicalDevice;
	data.device					= m_MainDevice.LogicalDevice;
	data.graphic_queue_index	= m_QueueFamilyIndices.GraphicsFamily;
	data.graphic_queue			= m_GraphicsQueue;
	data.imgui_descriptor_pool	= m_DescriptorsHandler.GetImguiDescriptorPool();
	data.min_image_count		= 3;	// setup correct practice
	data.image_count			= 3;	// setup correct practice
	data.render_pass			= m_RenderPassHandler.GetRenderPass();
	data.command_pool			= m_CommandHandler.GetCommandPool();
	data.command_buffers		= m_CommandHandler.GetCommandBuffers();

	return data;
}

const int VulkanRenderer::GetCurrentFrame() const
{
	return m_CurrentFrame;
}
