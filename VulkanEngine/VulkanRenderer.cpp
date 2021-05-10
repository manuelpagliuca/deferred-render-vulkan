#include "pch.h"

#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
	m_VulkanInstance					= 0;
	m_Surface							= 0;
	m_PushCostantRange					= {};
	m_graphicsQueue						= 0;
	m_presentationQueue					= 0;
	m_MainDevice.LogicalDevice			= 0;
	m_MainDevice.PhysicalDevice			= 0;
	m_UBOViewProjection.projection		= glm::mat4(1.f);
	m_UBOViewProjection.view			= glm::mat4(1.f);
	m_MainDevice.MinUniformBufferOffset	= 0;
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
		CreateInstance();
		CreateSurface();
		GetPhysicalDevice();
		CreateLogicalDevice();

		m_SwapChainHandler	 = SwapChainHandler(m_MainDevice, m_Surface, m_Window, m_QueueFamilyIndices);
		m_SwapChainHandler.CreateSwapChain();

		m_DescriptorsHandler = DescriptorsHandler(m_MainDevice.LogicalDevice);
		m_DescriptorsHandler.CreateViewProjectionDescriptorSetLayout();
		m_DescriptorsHandler.CreateTextureDescriptorSetLayout();

		m_RenderPassHandler  = RenderPassHandler(m_MainDevice, m_SwapChainHandler);
		CreatePushCostantRange();

		m_GraphicPipeline	 = GraphicPipeline(m_MainDevice, m_SwapChainHandler, &m_RenderPassHandler,
			m_DescriptorsHandler.GetViewProjectionDescriptorSetLayout(), 
			m_DescriptorsHandler.GetTextureDescriptorSetLayout(), m_PushCostantRange);

		Utility::CreateDepthBufferImage(m_DepthBufferImage, m_MainDevice, m_SwapChainHandler.GetExtent());

		m_SwapChainHandler.SetRenderPass(m_RenderPassHandler.GetRenderPassReference());

		m_SwapChainHandler.CreateFrameBuffers(m_DepthBufferImage.ImageView);

		m_CommandHandler = CommandHandler(m_MainDevice, &m_GraphicPipeline, m_RenderPassHandler.GetRenderPassReference());
		m_CommandHandler.CreateCommandPool(m_QueueFamilyIndices);
		m_CommandHandler.CreateCommandBuffers(m_SwapChainHandler.FrameBuffersSize());
		m_TextureObjects.CreateTextureSampler(m_MainDevice);

		CreateUniformBuffers();

		m_DescriptorsHandler.CreateDescriptorPool(m_SwapChainHandler.SwapChainImagesSize(), m_viewProjectionUBO.size());
		m_DescriptorsHandler.CreateDescriptorSets(m_viewProjectionUBO, sizeof(UboViewProjection), m_SwapChainHandler.SwapChainImagesSize());

		CreateSynchronisation();

		// Creazione delle matrici che verranno caricate sui Descriptor Sets
		m_UBOViewProjection.projection = glm::perspective(glm::radians(45.0f),
			static_cast<float>(m_SwapChainHandler.GetExtentWidth()) / static_cast<float>(m_SwapChainHandler.GetExtentHeight()),
			0.1f, 100.f);

		m_UBOViewProjection.view = glm::lookAt(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.0f, 0.f));

		// GLM threat Y-AXIS as Up-Axis, but in Vulkan the Y is down.
		m_UBOViewProjection.projection[1][1] *= -1;

		std::vector<Vertex> meshVertices =
		{
			{ { -0.4,  0.4,  0.0 }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } }, // 0
			{ { -0.4, -0.4,  0.0 }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } }, // 1
			{ {  0.4, -0.4,  0.0 }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }, // 2
			{ {  0.4,  0.4,  0.0 }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } }, // 3
		};

		std::vector<Vertex> meshVertices2 = {
			{ { -0.25,  0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },	// 0
			{ { -0.25, -0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }, // 1
			{ {  0.25, -0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, // 2
			{ {  0.25,  0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }, // 3
		};

		std::vector<uint32_t> meshIndices = {
			0, 1, 2,
			2, 3, 0
		};

		int giraffeTexture = Utility::CreateTexture(m_MainDevice, m_DescriptorsHandler.GetTexturePool(), m_DescriptorsHandler.GetTextureDescriptorSetLayout(), m_TextureObjects, m_graphicsQueue, m_CommandHandler.GetCommandPool(), "giraffe.jpg");

		m_MeshList.push_back(Mesh(
			m_MainDevice,
			m_graphicsQueue, m_CommandHandler.GetCommandPool(),
			&meshVertices, &meshIndices, giraffeTexture));

		m_MeshList.push_back(Mesh(
			m_MainDevice,
			m_graphicsQueue, m_CommandHandler.GetCommandPool(),
			&meshVertices2, &meshIndices, giraffeTexture));

		glm::mat4 meshModelMatrix = m_MeshList[0].getModel().model;
		meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(45.f), glm::vec3(.0f, .0f, 1.0f));
		m_MeshList[0].setModel(meshModelMatrix);
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

// Effettua l'operazioni di drawing nonchè di prelevamento dell'immagine.
// 1. Prelevala prossima immagine disponibile da disegnare e segnala il semaforo relativo quando abbiamo terminato
// 2. Invia il CommandBuffer alla Queue di esecuzione, assicurandosi che l'immagine da mettere SIGNALED sia disponibile
//    prima dell'operazione di drawing
// 3. Presenta l'immagine a schermo, dopo che il render è pronto.

void VulkanRenderer::Draw()
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

	m_CommandHandler.RecordCommands(imageIndex, m_SwapChainHandler.GetExtent(), m_SwapChainHandler.GetFrameBuffers(), m_MeshList, m_TextureObjects, m_DescriptorsHandler.GetDescriptorSets());

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
	result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_SyncObjects[m_CurrentFrame].InFlight);
	
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
	result = vkQueuePresentKHR(m_presentationQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		//m_SwapChainHandler.RecreateSwapChain();
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(m_Window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_MainDevice.LogicalDevice);

		m_SwapChainHandler.DestroyFrameBuffers();
		
		m_CommandHandler.FreeCommandBuffers();
		
		m_GraphicPipeline.DestroyPipeline();

		m_SwapChainHandler.DestroySwapChainImageViews();
		m_SwapChainHandler.DestroySwapChain();
		m_SwapChainHandler.IsRecreating(true);
		m_SwapChainHandler.CreateSwapChain();
		m_SwapChainHandler.IsRecreating(false);

		m_RenderPassHandler.CreateRenderPass();
		m_GraphicPipeline.CreateGraphicPipeline();
		Utility::CreateDepthBufferImage(m_DepthBufferImage, m_MainDevice, m_SwapChainHandler.GetExtent());
		m_SwapChainHandler.CreateFrameBuffers(m_DepthBufferImage.ImageView);
		m_CommandHandler.CreateCommandBuffers(m_SwapChainHandler.FrameBuffersSize());
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to present the image!");
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::CreateInstance()
{
	// Controllare se le Validation Layers sono abilitate
	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };				// Vettore contenente le Validation Layers (come cstring)

#ifdef ENABLED_VALIDATION_LAYERS
	if (!CheckValidationLayerSupport(&validationLayers))				// Controlla che le Validation Layers richieste siano effettivamente supportate
	{																							// (nel caso in cui siano state abilitate).
		throw std::runtime_error("VkInstance doesn't support the required validation layers");	// Nel caso in cui le Validation Layers non siano disponibili
	}
#endif // ENABLED_VALIDATION_LAYERS

	
	// APPLICATION INFO (per la creazione di un istanza di Vulkan si necessitano le informazioni a riguardo dell'applicazione)
	VkApplicationInfo appInfo = {};

	appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO; // Tipo di struttura
	appInfo.pApplicationName	= "Vulkan Render Application";		  // Nome dell'applicazione
	appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);			  // Versione personalizzata dell'applicazione
	appInfo.pEngineName			= "VULKAN RENDERER";				  // Nome dell'engine
	appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);			  // Versione personalizzata dell'engine
	appInfo.apiVersion			= VK_API_VERSION_1_0;				  // Versione di Vulkan che si vuole utilizzare (unico obbligatorio)

	// VULKAN INSTANCE
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	// Tipo di struttura
	createInfo.pApplicationInfo		= &appInfo;									// Puntatore alle informazioni dell'applicazione

	// Carico in un vettore le estensioni per l'istanza
	std::vector <const char*> instanceExtensions = std::vector<const char*>(); // Vettore che conterrà le estensioni (come cstring)
	LoadGlfwExtensions(instanceExtensions);									   // Preleva le estensioni di GLFW per Vulkan e le carica nel vettore
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);		   // Carico manualmente nel vettore l'estensione che permette di creare un debug messanger che verrà utilizzato per il Validation Layer (e altro...)

	if (!checkInstanceExtensionSupport(&instanceExtensions))								// Controlla se Vulkan supporta le estensioni presenti in instanceExtensions
	{
		throw std::runtime_error("VkInstance doesn't support the required extensions");		// Nel caso in cui le estensioni non siano supportate throwa un errore runtime
	}
	
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(instanceExtensions.size()); // Numero di estensioni abilitate
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();						   // Prende le estensioni abilitate (e supportate da Vulkan)
	
#ifdef ENABLED_VALIDATION_LAYERS
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());   // Numero di Validation Layers abilitate.
	createInfo.ppEnabledLayerNames = validationLayers.data();						   // Prende le Validation Layers abilitate (e supportate da Vulkan)
#else
	createInfo.enabledLayerCount = 0;								// Numero di Validation Layers abilitate.
	createInfo.ppEnabledLayerNames = nullptr;						// Prende le Validation Layers abilitate (e supportate da Vulkan)
#endif // ENABLED_VALIDATION_LAYERS

	VkResult res = vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance);					   // Crea un istanza di Vulkan.

	if (res != VK_SUCCESS) // Nel caso in cui l'istanza non venga creata correttamente, alza un eccezione runtime.
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}

#ifdef ENABLED_VALIDATION_LAYERS
	DebugMessanger::GetInstance()->SetupDebugMessenger(m_VulkanInstance);
#endif // ENABLED_VALIDATION_LAYERS
}


// Carica le estensioni di GLFW nel vettore dell'istanza di vulkan
void VulkanRenderer::LoadGlfwExtensions(std::vector<const char*>& instanceExtensions)
{
	uint32_t glfwExtensionCount = 0;	// GLFW potrebbe richiedere molteplici estensioni
	const char** glfwExtensions;		// Estensioni di GLFW (array di cstrings)

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // Funzione di GLFW che recupera le estensioni disponibili per Vulkan
	for (size_t i = 0; i < glfwExtensionCount; ++i)							 // Aggiunge le estensioni di GLFW a quelle dell'istanza di Vulkan
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}
}

// Controlla se le estensioni scaricate siano effettivamente supportate da Vulkan
bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* extensionsToCheck)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);			 // Preleviamo il numero di estensioni disponibili.
	
	std::vector<VkExtensionProperties> extensions(extensionCount);						 // Dimensioniamo un vettore con quella quantità.
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()); // Scarichiamo le estensioni supportate da Vulkan nel vettore

	for (const auto& checkExtension : *extensionsToCheck)			// Si controlla se le estensioni che si vogliono utilizzare
	{																// combaciano con le estensioni effettivamente supportate da Vulkan
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName))
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

// Controlla che le Validation Layers scaricate siano effettivamente supportate da Vulkan
bool VulkanRenderer::CheckValidationLayerSupport(std::vector<const char*>* validationLayers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);				 // Preleviamo il numero di ValidationLayer disponibili

	std::vector<VkLayerProperties> availableLayers(layerCount);				 // Dimensioniamo un vettore con quella quantità
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); // Scarichiamo le ValidationLayer all'interno del vettore

	for (const auto& layerName : *validationLayers)					// Controlliamo se le Validation Layers disponibili nel vettore
	{																// (quelle di Vulkan) siano compatibili con quelle richieste
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

void VulkanRenderer::GetPhysicalDevice()
{
	uint32_t deviceCount = 0;

	vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, nullptr);

	if (deviceCount == 0)										  
	{
		throw std::runtime_error("Can't find GPU that support Vulkan Instance!");
	}

	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, deviceList.data()); 

	// Mi fermo al primo dispositivo che supporta le QueueFamilies Grafiche
	for (const auto& device : deviceList)			
	{												
		if (checkDeviceSuitable(device))
		{
			m_MainDevice.PhysicalDevice = device;
			break;
		}
	}

	// Query delle proprtietà del dispositivo
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(m_MainDevice.PhysicalDevice, &deviceProperties);

	m_MainDevice.MinUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;// serve per DYNAMIC UBO

}

// Controlla se un device è adatto per l'applicazione
bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice possibleDevice)
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

// Crea il dispositivo logico
void VulkanRenderer::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { m_QueueFamilyIndices.GraphicsFamily , m_QueueFamilyIndices.PresentationFamily };

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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	deviceCreateInfo.pEnabledFeatures		 = &deviceFeatures;					// Features del dispositivo fisico che verranno utilizzate nel device logico (al momento nessuna).


	
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	VkResult result	= vkCreateDevice(m_MainDevice.PhysicalDevice, &deviceCreateInfo, nullptr, &m_MainDevice.LogicalDevice);	// Creo il device logico

	if (result != VK_SUCCESS)	// Nel caso in cui il Dispositivo Logico non venga creato con successo alzo un eccezione a runtime.
	{
		throw std::runtime_error("Failed to create Logical Device!");
	}

	vkGetDeviceQueue(							// Salvo il riferimento della queue grafica del device logico
		m_MainDevice.LogicalDevice,				// nella variabile m_graphicsQueue
		m_QueueFamilyIndices.GraphicsFamily, 
		0,
		&m_graphicsQueue);
		
	vkGetDeviceQueue(							 // Salvo il riferimento alla Presentation Queue del device logico
		m_MainDevice.LogicalDevice,				 // nella variabile 'm_presentationQueue'. Siccome è la medesima cosa della
		m_QueueFamilyIndices.PresentationFamily, // queue grafica, nel caso in cui sia presente una sola Queue nel device 
		0,										 // allora si avranno due riferimenti 'm_presentationQueue' e 'm_graphicsQueue' alla stessa queue
		&m_presentationQueue);
}

// Crea una Surface, dice a Vulkan come interfacciare le immagini con la finestra di GLFW
void VulkanRenderer::CreateSurface()
{
	VkResult res = glfwCreateWindowSurface(m_VulkanInstance, m_Window, nullptr, &m_Surface);
																		
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create the surface!");
	}
}

// Sincronizzazione : 1 semaforo per le immagini disponibili, 1 semaforo per le immagini presentabili/renderizzate, 1 fence per l'operazione di draw
void VulkanRenderer::CreateSynchronisation()
{
	// Oggetti di sincronizzazione quanti i frame in esecuzione
	m_SyncObjects.resize(MAX_FRAMES_IN_FLIGHT);

	// Semaphore
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;		// Specifica che viene creato nello stato di SIGNALED

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphoreCreateInfo, nullptr, &m_SyncObjects[i].ImageAvailable) != VK_SUCCESS ||
			vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphoreCreateInfo, nullptr, &m_SyncObjects[i].RenderFinished) != VK_SUCCESS ||
			vkCreateFence(m_MainDevice.LogicalDevice,	  &fenceCreateInfo,		nullptr, &m_SyncObjects[i].InFlight) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphores and/or Fence!");
		}
	}
}

void VulkanRenderer::CreatePushCostantRange()
{
	m_PushCostantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // Stage dove finiranno le push costant
	m_PushCostantRange.offset	  = 0;
	m_PushCostantRange.size		  = sizeof(Model);
}

void VulkanRenderer::CreateUniformBuffers()
{
	VkDeviceSize viewProjectionBufferSize = sizeof(m_UBOViewProjection);
	//VkDeviceSize modelBufferSize		  = m_modelUniformAlignment * MAX_OBJECTS;

	// Un UBO per ogni immagine della Swap Chain
	m_viewProjectionUBO.resize(m_SwapChainHandler.SwapChainImagesSize());
	m_viewProjectionUniformBufferMemory.resize(m_SwapChainHandler.SwapChainImagesSize());
	
	/*
	m_modelDynamicUBO.resize(m_swapChainImages.size());
	m_modelDynamicUniformBufferMemory.resize(m_swapChainImages.size());*/

	for (size_t i = 0; i < m_SwapChainHandler.SwapChainImagesSize(); i++)
	{
		Utility::CreateBuffer(m_MainDevice,	viewProjectionBufferSize,
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							  &m_viewProjectionUBO[i], &m_viewProjectionUniformBufferMemory[i]);
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

	// Distrugge le Meshes
	for (size_t i = 0; i < m_MeshList.size(); i++)
	{
		m_MeshList[i].destroyBuffers();
	}

	// Distrugge i Semafori
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
#endif // ENABLED_VALIDATION_LAYERS

	vkDestroyDevice(m_MainDevice.LogicalDevice, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);					
}