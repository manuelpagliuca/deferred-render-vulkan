#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
	m_debugMessenger = 0;
	m_descriptorPool = 0;
	m_descriptorSetLayout = 0;
	m_surface = 0;
	m_renderPass = 0;
	m_swapchain = 0;
	m_pipelineLayout = 0;
	m_instance = 0;
	m_graphicsPipeline = 0;
	m_graphicsComandPool = 0;
	m_graphicsQueue = 0;
	m_mainDevice.logicalDevice = 0;
	m_mainDevice.physicalDevice = 0;
	m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
	m_presentationQueue = 0;
	m_swapChainExtent.height = 0;
	m_swapChainExtent.width = 0;
	m_mvp.model = glm::mat4(1.f);
	m_mvp.projection = glm::mat4(1.f);
	m_mvp.view = glm::mat4(1.f);
}

int VulkanRenderer::init(void* t_window)
{
	if (!t_window)
	{
		std::cerr << "Unable to transfer GLFW window" << std::endl;
		return EXIT_FAILURE;
	}

	m_window = static_cast<GLFWwindow*>(t_window);

	try
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createDescriptorSetLayout();
		createRenderPass();
		createGraphicPipeline();
		createFramebuffers();
		createCommandPool();

		// Creazione delle matrici che verranno caricate sui Descriptor Sets
		m_mvp.projection = glm::perspective(glm::radians(45.0f),
											static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height),
											0.1f, 100.f);

		m_mvp.view = glm::lookAt(glm::vec3(3.f, 1.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.0f, 0.f));
		
		m_mvp.model = glm::mat4(1.0f);

		// GLM threat Y-AXIS as Up-Axis, but in Vulkan the Y is down.
		m_mvp.projection[1][1] *= -1;

		// Creazione della mesh
		std::vector<Vertex> meshVertices = {
			{ { -0.1, -0.4, 0.0 }, { 1.0f, 0.0f, 0.0f } },	// 0
			{ { -0.1, 0.4, 0.0  }, { 0.0f, 1.0f, 0.0f } },	// 1
			{ { -0.9, 0.4, 0.0  }, { 0.0f, 0.0f, 1.0f } },  // 2
			{ { -0.9, -0.4, 0.0 }, { 1.0f, 1.0f, 0.0f } },  // 3
		};

		std::vector<Vertex> meshVertices2 = {
			{ { 0.9, -0.3, 0.0 }, { 1.0f, 0.0f, 0.0f } },	// 0
			{ { 0.9,  0.1, 0.0 }, { 0.0f, 1.0f, 0.0f } },	// 1
			{ { 0.1,  0.3, 0.0 }, { 0.0f, 0.0f, 1.0f } },   // 2
			{ { 0.1, -0.3, 0.0 }, { 1.0f, 1.0f, 0.0f } },   // 3
		};

		// Index Data
		std::vector<uint32_t> meshIndices = {
			0, 1, 2,
			2, 3, 0
		};

		m_meshList.push_back(Mesh(
			m_mainDevice.physicalDevice, m_mainDevice.logicalDevice,
			m_graphicsQueue, m_graphicsComandPool,
			&meshVertices, &meshIndices));

		m_meshList.push_back(Mesh(
			m_mainDevice.physicalDevice, m_mainDevice.logicalDevice,
			m_graphicsQueue, m_graphicsComandPool,
			&meshVertices2, &meshIndices));

		createCommandBuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		recordCommands();
		createSynchronisation();
	}
	catch (std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}

void VulkanRenderer::updateModel(glm::mat4 newModel)
{
	m_mvp.model = newModel;
}

// Effettua l'operazioni di drawing nonchè di prelevamento dell'immagine.
// 1. Prelevala prossima immagine disponibile da disegnare e segnala il semaforo relativo quando abbiamo terminato
// 2. Invia il CommandBuffer alla Queue di esecuzione, assicurandosi che l'immagine da mettere SIGNALED sia disponibile
//    prima dell'operazione di drawing
// 3. Presenta l'immagine a schermo, dopo che il render è pronto.

void VulkanRenderer::draw()
{
	// Aspetta 1 Fence di essere nello stato di SIGNALED 
	// Aspettare per il dato Fence il segnale dell'ultimo draw effettuato prima di continuare
	vkWaitForFences(m_mainDevice.logicalDevice, 1, &m_syncObjects[m_currentFrame].m_inFlight, VK_TRUE, UINT64_MAX);

	// Metto la Fence ad UNSIGNALED (la GPU deve aspettare per la prossima operazione di draw)
	vkResetFences(m_mainDevice.logicalDevice, 1, &m_syncObjects[m_currentFrame].m_inFlight);
		
	// Recupero l'index della prossima immagine disponibile della SwapChain,
	// e mette SIGNALED il relativo semaforo 'm_imageAvailable' per avvisare
	// che l'immagine è pronta ad essere utilizzata.
	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_mainDevice.logicalDevice, m_swapchain,
						  std::numeric_limits<uint64_t>::max(),
					      m_syncObjects[m_currentFrame].m_imageAvailable, VK_NULL_HANDLE, &imageIndex);

	// Copia nell'UniformBuffer della GPU le matrici MVP
	updateUniformFunctions(imageIndex);

	// Stages dove aspettare che il semaforo sia SIGNALED (all'output del final color)
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	
	};
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	submitInfo.waitSemaphoreCount	= 1;											   // Numero dei semafori da aspettare
	submitInfo.pWaitSemaphores		= &m_syncObjects[m_currentFrame].m_imageAvailable; // Semaforo di disponibilità dell'immagine (aspetta che sia SIGNALED)
	submitInfo.pWaitDstStageMask	= waitStages;									   // Stage dove iniziare la semaphore wait (output del final color, termine della pipeline)
	
	submitInfo.commandBufferCount	= 1;								 // Numero di Command Buffer da inviare alla Queue
	submitInfo.pCommandBuffers		= &m_commandBuffer[imageIndex];		 // Il Command Buffer da inviare non è quello corrente, ma è quello dell'immagine disponile
	
	submitInfo.signalSemaphoreCount = 1;												// Numero di semafori a cui segnalare na volta che il CommandBuffer ha terminato
	submitInfo.pSignalSemaphores	= &m_syncObjects[m_currentFrame].m_renderFinished; // Semaforo di fine Rendering (verrà messo a SIGNALED)


	// L'operazione di submit del CommandBuffer alla Queue accade se prima del termine della Pipeline
	// è presente una nuova immagine disponibile, cosa garantita trammite una semaphore wait su 'm_imageAvailable' 
	// posizionata proprio prima dell'output stage della Pipeline.
	// Una volta che l'immagine sarà disponibile verranno eseguite le operazioni del CommandBuffer 
	// sulla nuova immagine disponibile. Al termine delle operazioni del CommandBuffer il nuovo render sarà pronto
	// e verrà avvisato con il semaforo 'm_renderFinished'.
	// Inoltre al termine del render verrà anche avvisato il Fence, per dire che è possibile effettuare
	// l'operazione di drawing a schermo
	VkResult result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_syncObjects[m_currentFrame].m_inFlight);
	
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit Command Buffer to Queue!");
	}

	// Presentazione dell'immagine a schermo
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType			   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;									// Numero di semafori da aspettare prima di renderizzare
	presentInfo.pWaitSemaphores	   = &m_syncObjects[m_currentFrame].m_renderFinished; // Aspetta che il rendering dell'immagine sia terminato
	presentInfo.swapchainCount	   = 1;									// Numero di swapchains a cui presentare
	presentInfo.pSwapchains		   = &m_swapchain;	 					// Swapchain contenente le immagini
	presentInfo.pImageIndices	   = &imageIndex;						// Indice dell'immagine nella SwapChain da visualizzare (quella nuova pescata dalle immagini disponibili)

	// Prima di effettuare l'operazione di presentazione dell'immagine a schermo si deve assicurare
	// che il render della nuova immagine sia pronto, cosa che viene assicurata dal semaforo 'm_renderFinished' del frame corrente.
	// Allora visto che l'immagine è ufficialmente pronta verrà mostrata a schermo
	result = vkQueuePresentKHR(m_presentationQueue, &presentInfo);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present the image!");
	}

	//vkQueueWaitIdle(m_presentationQueue);

	// Passa al prossimo frame
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::createInstance()
{
	// Controllare se le Validation Layers sono abilitate
	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };				// Vettore contenente le Validation Layers (come cstring)

	if (enableValidationLayers && !checkValidationLayerSupport(&validationLayers))				// Controlla che le Validation Layers richieste siano effettivamente supportate
	{																							// (nel caso in cui siano state abilitate).
		throw std::runtime_error("VkInstance doesn't support the required validation layers");	// Nel caso in cui le Validation Layers non siano disponibili
	}
	
	// APPLICATION INFO (per la creazione di un istanza di Vulkan si necessitano le informazioni a riguardo dell'applicazione)
	VkApplicationInfo appInfo = {};

	appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO; // Tipo di struttura
	appInfo.pApplicationName	= "Vulkan Render Application";		  // Nome dell'applicazione
	appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);			  // Versione personalizzata dell'applicazione
	appInfo.pEngineName			= "RED_RENDERER";					  // Nome dell'engine
	appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);			  // Versione personalizzata dell'engine
	appInfo.apiVersion			= VK_API_VERSION_1_0;				  // Versione di Vulkan che si vuole utilizzare (unico obbligatorio)

	// VULKAN INSTANCE
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	// Tipo di struttura
	createInfo.pApplicationInfo		= &appInfo;									// Puntatore alle informazioni dell'applicazione

	// Carico in un vettore le estensioni per l'istanza
	std::vector <const char*> instanceExtensions = std::vector<const char*>(); // Vettore che conterrà le estensioni (come cstring)
	loadGlfwExtensions(instanceExtensions);									   // Preleva le estensioni di GLFW per Vulkan e le carica nel vettore
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);		   // Carico manualmente nel vettore l'estensione che permette di creare un debug messanger che verrà utilizzato per il Validation Layer (e altro...)

	if (!checkInstanceExtensionSupport(&instanceExtensions))								// Controlla se Vulkan supporta le estensioni presenti in instanceExtensions
	{
		throw std::runtime_error("VkInstance doesn't support the required extensions");		// Nel caso in cui le estensioni non siano supportate throwa un errore runtime
	}
	
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(instanceExtensions.size()); // Numero di estensioni abilitate
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();						   // Prende le estensioni abilitate (e supportate da Vulkan)
	
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());   // Numero di Validation Layers abilitate.
		createInfo.ppEnabledLayerNames = validationLayers.data();						   // Prende le Validation Layers abilitate (e supportate da Vulkan)
	}
	else
	{
		createInfo.enabledLayerCount   = 0;   // Numero di Validation Layers abilitate.
		createInfo.ppEnabledLayerNames = nullptr;						   // Prende le Validation Layers abilitate (e supportate da Vulkan)
	}
	
	VkResult res = vkCreateInstance(&createInfo, nullptr, &m_instance);					   // Crea un istanza di Vulkan.

	if (res != VK_SUCCESS) // Nel caso in cui l'istanza non venga creata correttamente, alza un eccezione runtime.
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}
}

// Carica le estensioni di GLFW nel vettore dell'istanza di vulkan
void VulkanRenderer::loadGlfwExtensions(std::vector<const char*>& instanceExtensions)
{
	uint32_t glfwExtensionCount = 0;	// GLFW potrebbe richiedere molteplici estensioni
	const char** glfwExtensions;		// Estensioni di GLFW (array di cstrings)

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // Funzione di GLFW che recupera le estensioni disponibili per Vulkan
	for (size_t i = 0; i < glfwExtensionCount; ++i)							 // Aggiunge le estensioni di GLFW a quelle dell'istanza di Vulkan
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}
}

void VulkanRenderer::recordCommands()
{
	// Informazioni sul come inizializzare ogni Command Buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
	// Informazioni sul come inizializzare il RenderPass (necessario solo per le applicazioni grafiche)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType			  = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; 
	renderPassBeginInfo.renderPass		  = m_renderPass;	   // RenderPass
	renderPassBeginInfo.renderArea.offset = { 0, 0 };		   // Start point del RenderPass (in Pixels)
	renderPassBeginInfo.renderArea.extent = m_swapChainExtent; // Dimensione delal regione dove eseguire il RenderPass (partendo dall'offset)

	// Valori che vengono utilizzati per pulire l'immagine (background color)
	VkClearValue clearValues[] = {
		{0.6f, 0.65f, 0.4f, 1.0f}
	};

	renderPassBeginInfo.pClearValues	= clearValues; // Puntatore ad un array di ClearValues (TODO: Depth Attachment)
	renderPassBeginInfo.clearValueCount = 1;		   // Numero di ClearValues

	// Per ogni Command Buffer
	for (size_t i = 0; i < m_commandBuffer.size(); ++i)
	{
		// Associo un Framebuffer per volta
		renderPassBeginInfo.framebuffer = m_swapChainFrameBuffers[i];

		// Inizia a registrare i comandi nel Command Buffer
		VkResult res = vkBeginCommandBuffer(m_commandBuffer[i], &bufferBeginInfo);
		
		if (res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to start recording a Command Buffer!");
		}

			// Inizio del RenderPass
			// VK_SUBPASS_CONTENTS_INLINE indica che tutti i comandi verranno registrati direttamente sul CommandBuffer primario
			// e che il CommandBuffer secondario non debba essere eseguito all'interno del Subpass
			vkCmdBeginRenderPass(m_commandBuffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				// Binding della Pipeline Grafica ad un Command Buffer.
				// VK_PIPELINE_BIND_POINT_GRAPHICS, indica il tipo Binding Point della Pipeline (in questo grafico per la grafica).
				// (È possibile impostare molteplici pipeline e switchare, per esempio una versione DEFERRED o WIREFRAME)
				vkCmdBindPipeline(m_commandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
				
				for (size_t j = 0; j < m_meshList.size(); ++j)
				{
					VkBuffer vertexBuffers[] = { m_meshList[j].getVertexBuffer() }; // Buffer da bindare prima di essere disegnati
					VkDeviceSize offsets[]   = { 0 };
					
					// Binding del Vertex Buffer di una Mesh ad un Command Buffer
					vkCmdBindVertexBuffers(m_commandBuffer[i], 0, 1, vertexBuffers, offsets);

					// Bind del Index Buffer di una Mesh ad un Command Buffer, con offset 0 ed usando uint_32
					vkCmdBindIndexBuffer(m_commandBuffer[i], m_meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

					// Binding del Descriptor Set ad un Command Buffer
					vkCmdBindDescriptorSets(m_commandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);

					// Invia un Comando di IndexedDraw ad un Command Buffer
					vkCmdDrawIndexed(m_commandBuffer[i], m_meshList[j].getIndexCount(), 1, 0, 0, 0);
				}

			// Termine del Render Pass
			vkCmdEndRenderPass(m_commandBuffer[i]);

		// Termian la registrazione dei comandi
		res = vkEndCommandBuffer(m_commandBuffer[i]);

		if (res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to stop recording a Command Buffer!");
		}
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
		{
			return false;
		}
	}

	return true;
}

// Controlla che le Validation Layers scaricate siano effettivamente supportate da Vulkan
bool VulkanRenderer::checkValidationLayerSupport(std::vector<const char*>* validationLayers)
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

void VulkanRenderer::getPhysicalDevice()
{
	uint32_t deviceCount = 0;

	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)										  
	{
		throw std::runtime_error("Can't find GPU that support Vulkan Instance!");
	}

	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, deviceList.data()); 

	// Mi fermo al primo dispositivo che supporta le QueueFamilies Grafiche
	for (const auto& device : deviceList)			
	{												
		if (checkDeviceSuitable(device))
		{
			m_mainDevice.physicalDevice = device;
			break;
		}
	}
}

// Controlla se un device è adatto per l'applicazione
bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	/* Al momento non ci interessano particolari caratteristiche della GPU

	// Informazioni generiche a riguardo del dispositivo
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Informazioni rispetto ai servizi che offre il dispositvo
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	// Preleva dal dispositivo fisico gli indici delle QueueFamily per la Grafica e la Presentazione
	getQueueFamiliesIndices(device);

	// Controlla che le estensioni richieste siano disponibili nel dispositivo fisico
	bool const extensionSupported = checkDeviceExtensionSupport(device);


	bool swapChainValid	= false;

	// Se le estensioni richieste sono supportate (quindi Surface compresa), si procede con la SwapChain
	if (extensionSupported)
	{						
		SwapChainDetails swapChainDetails = getSwapChainDetails(device);
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}

	return m_queueFamilyIndices.isValid() && extensionSupported && swapChainValid;	// Il dispositivo è considerato adatto se :
																					// 1. Gli indici delle sue Queue Families sono validi
																					// 2. Se le estensioni richieste sono supportate
																					// 3. Se la Swap Chain è valida 
}

// Controllo l'esistenza di estensioni all'interno del device fisico
bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Prelevo il numero di estensioni supportate nel dispositivo fisico
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0)															
		return false;

	std::vector<VkExtensionProperties> physicalDeviceExtensions(extensionCount);						
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, physicalDeviceExtensions.data());

	for (const auto& extensionToCheck : m_requestedDeviceExtensions)				
	{																	
		bool hasExtension = false;

		for (const auto& extension : physicalDeviceExtensions)
		{
			if (strcmp(extensionToCheck, extension.extensionName) == 0)
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

// Sceglie il miglior formato immagine per la Surface, esso è soggettivo ma ci sono delle pratiche standard
// Formato			:	VK_FORMAT_R8G8B8A8_UNORM  (backup value : VK_FORMAT_B8G8R8A8_UNORM)
// Spazio colore	:	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)		// Se è disponibile solo un formato ed è undefined
	{																			// significa che sono disponibili tutti i formati (no restrictions).
		return { VK_FORMAT_R8G8B8A8_UNORM , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// Altrimenti significa che è presente un numero ristretto di formati
	// Allora si cerca per quello ottimale (RGBA o BGRA)
	for (const auto& format : formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) 
			&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}

// Se è presenta la modalità MailBox (Triple Buffer) restituisce quella, altrimenti restituisce la FIFO (sempre garantita)
VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	for(const auto &presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// Se la corrente Extent presente nella surface ha una
	// dimensione differente dal limite numerico, allora è contenuta la dimensione corretta della finestra
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;			
	}
	else				// Altrimenti, significa che è ancora presente un valore di default
	{
		int width;
		int height;
		glfwGetFramebufferSize(m_window, &width, &height);	// Si prelevano le dimensioni della finestra di GLFW
															// Si crea una nuova Extent con le dimensioni corrette
		VkExtent2D newExtent = {};
		newExtent.width		 = static_cast<uint32_t>(width);
		newExtent.height	 = static_cast<uint32_t>(height);

		newExtent.width		 = std::max(surfaceCapabilities.minImageExtent.width,
							   std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		
		newExtent.height	 = std::max(surfaceCapabilities.minImageExtent.height,
							   std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}

// Crea il dispositivo logico
void VulkanRenderer::createLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { m_queueFamilyIndices.graphicsFamily , m_queueFamilyIndices.presentationFamily };

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
	deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(m_requestedDeviceExtensions.size());	// Numero di estensioni da utilizzare sul dispositivo logico.
	deviceCreateInfo.ppEnabledExtensionNames = m_requestedDeviceExtensions.data();							// Puntatore ad un array che contiene le estensioni abilitate (SwapChain, ...).
	deviceCreateInfo.pEnabledFeatures		 = new VkPhysicalDeviceFeatures{};					// Features del dispositivo fisico che verranno utilizzate nel device logico (al momento nessuna).

	VkResult result	= vkCreateDevice(m_mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &m_mainDevice.logicalDevice);	// Creo il device logico

	if (result != VK_SUCCESS)	// Nel caso in cui il Dispositivo Logico non venga creato con successo alzo un eccezione a runtime.
	{
		throw std::runtime_error("Failed to create Logical Device!");
	}

	vkGetDeviceQueue(							// Salvo il riferimento della queue grafica del device logico
		m_mainDevice.logicalDevice,				// nella variabile m_graphicsQueue
		m_queueFamilyIndices.graphicsFamily, 
		0,
		&m_graphicsQueue);
		
	vkGetDeviceQueue(							 // Salvo il riferimento alla Presentation Queue del device logico
		m_mainDevice.logicalDevice,				 // nella variabile 'm_presentationQueue'. Siccome è la medesima cosa della
		m_queueFamilyIndices.presentationFamily, // queue grafica, nel caso in cui sia presente una sola Queue nel device 
		0,										 // allora si avranno due riferimenti 'm_presentationQueue' e 'm_graphicsQueue' alla stessa queue
		&m_presentationQueue);
}

// Crea una Surface, dice a Vulkan come interfacciare le immagini con la finestra di GLFW
void VulkanRenderer::createSurface()
{
	VkResult res = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
																		
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create the surface!");
	}
}

// Crea la SwapChain e la salva all'interno del Renderer come 'm_swapChain'.
// Crea le Image Views e le carica assieme alle Image all'interno del vettore 'm_swapChainImages'
void VulkanRenderer::createSwapChain()
{
	// Preleva le informazioni della SwapChain (Capabilities, Formats, Presentation modes) a partire dalla Surface
	SwapChainDetails swapChainDetails = getSwapChainDetails(m_mainDevice.physicalDevice); 

	// Dalle precedenti informazioni seleziono le impostazioni migliori per la SwapChain 
	VkExtent2D extent				 = chooseSwapExtent(swapChainDetails.surfaceCapabilities);
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode	 = chooseBestPresentationMode(swapChainDetails.presentationModes);
	
	// Il numero di immagini è 1 immagine in più della quantità minima di immagini per il triple buffering (un immagine trattenuta nella queue)
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

	// Se il numero massimo di immagini è positivo e questo numero è minore delle immagini che verranno utilizzate
	// Allora significa che il massimo numero di immagini disponibile non basta per supportare il triple-buffering
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && 
		swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType					 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;			  
	swapChainCreateInfo.surface					 = m_surface;											  // Surface alla quale viene bindata la SwapChain
	swapChainCreateInfo.imageFormat				 = surfaceFormat.format;								  // Formato delle immagini della SwapChain
	swapChainCreateInfo.imageColorSpace			 = surfaceFormat.colorSpace;							  // Spazio colore delle immagini della SwapChain
	swapChainCreateInfo.presentMode				 = presentMode;											  // Presentation Mode della SwapChain
	swapChainCreateInfo.imageExtent				 = extent;												  // Extent (dimensioni in pixels) delle immagini della SwapChain 
	swapChainCreateInfo.minImageCount			 = imageCount;											  // Minimo numero di immagini utilizzate dalla SwapChain
	swapChainCreateInfo.imageArrayLayers		 = 1;													  // Numero di viste per una Surface multiview/stereo
	swapChainCreateInfo.imageUsage				 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;					  // Specifica l'utilizzo dell'immagine (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : le Image sono utilizzate per creare ImageView adatte ai colori)
	swapChainCreateInfo.preTransform			 = swapChainDetails.surfaceCapabilities.currentTransform; // Trasformazione relativa all'orientamento del motore di presentazione (se non combacia con la currentTransform della Surface viene calcolata durante la presentazione)
	swapChainCreateInfo.compositeAlpha			 = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;					  // Come vengono gestite le immagini trasparenti con Surface esterne (e.g., 2 finestre che si sovrappongono)
	swapChainCreateInfo.clipped					 = VK_TRUE;												  // Abilitare il clipping tra la Surface e l'ambiente esterno

	// Se le famiglie Graphics e Presentation sono diverse, allora la SwapChain deve trattare le immagini
	// in maniera condivisa e concorrente tra le due queues (non è la modalità più efficente).
	if (m_queueFamilyIndices.graphicsFamily != m_queueFamilyIndices.presentationFamily)
	{
		uint32_t queueFamilyIndices[]{
			static_cast<int32_t>(m_queueFamilyIndices.graphicsFamily),
			static_cast<int32_t>(m_queueFamilyIndices.presentationFamily)
		};

		swapChainCreateInfo.imageSharingMode	  = VK_SHARING_MODE_CONCURRENT;	// Modalità concorrente e condivisa
		swapChainCreateInfo.queueFamilyIndexCount = 2;							// Numero di queue utilizzate
		swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;			// Array delle contenente i riferimenti alle Queue
	}
	else
	{
		swapChainCreateInfo.imageSharingMode	  = VK_SHARING_MODE_EXCLUSIVE; // Modalità esclusiva (1 sola queue)
		swapChainCreateInfo.queueFamilyIndexCount = 1;						   // Numero di queue utilizzate
		swapChainCreateInfo.pQueueFamilyIndices   = nullptr;				   // Array delle contenente i riferimenti alle Queue
	}

	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // La SwapChain viene sostituita dall'attuale, è possibile fornire un link per trasferirne la responsabilità

	// Creazione della SwapChain e memorizzazione nel Renderer dentro 'm_swapchain'
	VkResult res = vkCreateSwapchainKHR(m_mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &m_swapchain);
	
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a SwapChain!");
	}

	// Salviamo dei riferimenti relativi al formato e all'extent (servirà nella creazione del RenderPass, TODO)
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent	   = extent;

	// Salviamo le Image pre-esistenti all'interno della SwapChain in un vettore
	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapchain, &swapChainImageCount, nullptr);

	std::vector <VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapchain, &swapChainImageCount, images.data());

	// Partendo dalle Image pre-esistenti, costruiamo delle immagini che hanno la stessa Image ed una relativa ImageView
	// e le aggiungiamo al vettore interno al Renderer 'm_swapChainImages'.
	for (VkImage image : images)
	{
		SwapChainImage swapChainImage = {};

		swapChainImage.image	 = image;
		swapChainImage.imageView = createImageView(image, m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		m_swapChainImages.push_back(swapChainImage);
	}
}

// Creazione di una ImageView partendo da Image, Formato e Aspetto da visualizzare (siamo interessati al colore)
VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspetFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
	viewCreateInfo.image		= image;									// Immagine pre-esistente nella SwapChain
	viewCreateInfo.viewType		= VK_IMAGE_VIEW_TYPE_2D;					// Tipo delle immagini (2D)
	viewCreateInfo.format		= format;									// Formato delle immagini (stesso della Surface)
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;			// Imposto lo swizzle con il valore della componente effettiva
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;			
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;			
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;			

	// L'intervallo delle sottorisorse dicono all'ImageView quale parte dell'immagine visualizzare
	viewCreateInfo.subresourceRange.aspectMask     = aspetFlags; // Quale aspetto dell'immagine visualizzare (COLOR, DEPTH, STENCIL)
	viewCreateInfo.subresourceRange.baseMipLevel   = 0;			 // Livello iniziale della mipmap (primo mipmap level)
	viewCreateInfo.subresourceRange.levelCount     = 1;			 // Numero di livelli mipmap da visualizzare
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;			 // Livello iniziale del primo arrayLayer (primo arrayLayer)
	viewCreateInfo.subresourceRange.layerCount	   = 1;			 // Numero di ArrayLayer

	VkImageView imageView;
	VkResult res = vkCreateImageView(m_mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
	
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image View!");
	}

	return imageView;
}

// Preleva gli indici delle Queue Family di Grafica e Presentazione
void VulkanRenderer::getQueueFamiliesIndices(VkPhysicalDevice device)
{
	// Prelevo le Queue Families disponibili nel dispositivo fisico
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);				 
	
	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);						 
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	// Indice delle Queue Families (a carico del developer)
	int queueIndex = 0;

	if (queueFamilyCount > 0)
	{
		for (const auto& queueFamily : queueFamilyList)
		{
			// Se la QueueFamily è valida ed è una Queue grafica, salvo l'indice nel Renderer
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)	
			{																					
				m_queueFamilyIndices.graphicsFamily = queueIndex;
			}

			VkBool32 presentationSupport = false;

			/*	Query per chiedere se è supportata la Presentation Mode nella Queue Family.
				Solitamente le Queue Family appartenenti alla grafica la supportano.
				Questa caratteristica è obbligatoria per presentare l'immagine dalla SwapChain alla Surface.*/
			vkGetPhysicalDeviceSurfaceSupportKHR(device, queueIndex, m_surface, &presentationSupport);

			// Se la Queue Family supporta la Presentation Mode, salvo l'indice nel Renderer (solitamente sono gli stessi indici)
			if (presentationSupport)
			{
				m_queueFamilyIndices.presentationFamily = queueIndex;
			}

			if (m_queueFamilyIndices.isValid())
			{
				break;
			}

			++queueIndex;
		}
	}
}

// Restituisce le informazioni riguardanti la SwapChain (Surface Capabilities, Formati supportati, Presentation Modes supportate)
SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
	// Surface Capabilities, Surface Image Formats e Presentation Modes
	SwapChainDetails swapChainDetails;

	// Query per le proprietà (capabilities) della Surface 
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &swapChainDetails.surfaceCapabilities);

	// Scarica in un'array delle coppie <Formato Immagine, Spazio Colore> che sono supportate dalla Surface
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount > 0)								
	{													
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, swapChainDetails.formats.data());
	}

	// Preleva le Presentation Mode supportate dalla Surface
	uint32_t presentationCount = 0;				 
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, nullptr);

	if (presentationCount > 0)											
	{
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, swapChainDetails.presentationModes.data());
	}

	return swapChainDetails;
}

// Impostiamo debug messanger (possibile solo con l'estensione VK_EXT_DEBUG_UTILS_EXTENSION_NAME e con le Validation Layers abilitate)
void VulkanRenderer::setupDebugMessenger()
{
	if (!enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType		   = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;	// Il tipo della struttura
	createInfo.flags		   = 0;
	createInfo.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |			// Imposta i tipi di severità a cui siamo interessati
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType	   = //VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT	|			// Imposta il tipo di messaggio disponibili
								 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = debugCallback;	// Imposta la funzione di callback da chiamare in caso di errore
	createInfo.pUserData	   = nullptr;  		// Dati extra da passare all'interno della callback

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) // Creo il debug messanger, in caso di errore alzo un eccezione
	{
		throw std::runtime_error("Failed to set up debug messenger!");
	}
}

// Funzione di Callback del DebugMessanger
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	switch (messageType)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		std::cerr << "-[General]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		std::cerr << "-[Validation]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		std::cerr << "-[Performance]";
		break;
	}

	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		std::cerr << " [Warning]";
		break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		std::cerr << " [Verbose]";
		break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		std::cerr << " [Error]";
		break;
	}

	std::cerr << pCallbackData->pMessage << "\n\n";

	return VK_FALSE;
}

// Alloca il DebugMessanger (preleva il comando dall'istanza di Vulkan)
VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(
	VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)						// Le informazioni contenute nella createInfo devono essere passate alla funzione
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");	// "vkCreateDebugUtilsMessengerEXT", ma per sua natura (extension function) non è caricata
																			// nell'API. Si utilizza "vkGetInstanceProcAddr(...)" per recuperarne l'indirizzo e crearne una proxy function.
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// Distrugge il DebugMessanger (preleva il comando dall'istanza di Vulkan)
void VulkanRenderer::DestroyDebugUtilsMessengerEXT(
	VkInstance instance, 
	VkDebugUtilsMessengerEXT m_debugMessenger, 
	const VkAllocationCallbacks* pAllocator) 
{
	
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)						// Per distruggere il debug messanger si deve utilizzare una funzione
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");	// che non è esposta nelle API, "vkDestroyDebugUtilsMessengerEXT".
	
	if (func != nullptr) 
	{
		func(instance, m_debugMessenger, pAllocator);
	}
}

void VulkanRenderer::createGraphicPipeline()
{
	/* Shader Modules */

	// Lettura codici intermedi SPIR-V
	std::vector<char> vertexShaderCode	 = readFile("./shaders/vert.spv");
	std::vector<char> fragmentShaderCode = readFile("./shaders/frag.spv");

	// Shader Modules (Vertex & Fragment)
	VkShaderModule vertexShaderModule	 = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule  = createShaderModule(fragmentShaderCode);

	/* Shader Stages */

	// Vertex Stage
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;	// Vertex Shader
	vertexShaderCreateInfo.module = vertexShaderModule;			// ShaderModule associato
	vertexShaderCreateInfo.pName  = "main";						// Entry point all'interno del VertexShader

	// Fragment Stage
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage	= VK_SHADER_STAGE_FRAGMENT_BIT;	// Fragment Shader
	fragmentShaderCreateInfo.module	= fragmentShaderModule;			// ShaderModule associato
	fragmentShaderCreateInfo.pName	= "main";						// Entry point all'interno del FragmentShader

	// Vettore di ShaderStage, successivamente richiesti dalla Pipeline Grafica
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };


	/* Vertex Input Binding Description per i Vertex */
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding   = 0;							// Indice di Binding (possono essere presenti molteplici stream di binding)
	bindingDescription.stride    = sizeof(Vertex);				// Scostamento tra gli elementi in input forniti dal Vertex
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Rateo con cui i vertex attributes sono presi dai buffers TODO
																// VK_VERTEX_INPUT_RATE_VERTEX	 : Si sposta al prossimo vertice
																// VK_VERTEX_INPUT_RATE_INSTANCE : Si sposta al prossimo vertice per la prossima istanza
																
	// Attribute Descriptions, come interpretare il Vertex Input su un dato binding stream
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;
	
	// Attribute for the Vertex Position
	attributeDescriptions[0].binding  = 0;						    // Stream di binding al quale si riferisce
	attributeDescriptions[0].location = 0;						    // Locazione del binding nel Vertex Shader
	attributeDescriptions[0].format	  = VK_FORMAT_R32G32B32_SFLOAT; // Formato e dimensione dell'attributo (32-bit per ogni canale di signed floating point)
	attributeDescriptions[0].offset	  = offsetof(Vertex, pos);		// Offset della posizione all'interno della struct 'Vertex'

	// Attribute for the Vertex Color
	attributeDescriptions[1].binding  = 0;						    // Stream di binding al quale si riferisce
	attributeDescriptions[1].location = 1;						    // Locazione del binding nel Vertex Shader
	attributeDescriptions[1].format	  = VK_FORMAT_R32G32B32_SFLOAT; // Formato e dimensione dell'attributo (32-bit per ogni canale di signed floating point)
	attributeDescriptions[1].offset   = offsetof(Vertex, col);		// Offset del colore all'interno della struct 'Vertex'

	/* Graphic Pipeline */

	// #1 STAGE - VERTEX INPUT
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType							  = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount	  = 1;												 	 // Numero di Vertex Binding Descriptions
	vertexInputCreateInfo.pVertexBindingDescriptions	  = &bindingDescription;								 // Puntatore ad un array di Vertex Binding Description
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); // Numero di Attribute Descriptions
	vertexInputCreateInfo.pVertexAttributeDescriptions	  = attributeDescriptions.data();						 // Puntatore ad un array di Attribute Descriptions

	// #2 Stage - INPUT ASSEMBLY
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType				   = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology			   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Tipo della primitiva con cui assemblare i vertici
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;							  // Abilità di ripartire con una nuova strip

	// #3 Stage - VIEWPORT & SCISSOR
	
	// Viewport
	VkViewport viewport = {};
	viewport.x			= 0.f;											// Valore iniziale della viewport-x
	viewport.y			= 0.f;											// Valore iniziale della viewport-y
	viewport.width		= static_cast<float>(m_swapChainExtent.width);	// Larghezza della viewport
	viewport.height		= static_cast<float>(m_swapChainExtent.height);	// Altezza della viewport
	viewport.minDepth	= 0.0f;											// Minima profondità del framebuffer
	viewport.maxDepth	= 1.0f;											// Massima profondità del framebuffer
	
	// Scissor
	VkRect2D scissor = {};				  
	scissor.offset	 = { 0,0 };			  // Offset da cui iniziare a tagliare la regione
	scissor.extent	 = m_swapChainExtent; // Extent che descrive la regione da tagliare

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType		  = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;			// Numero di Viewport
	viewportStateCreateInfo.pViewports	  = &viewport;  // Puntatore ad un array di viewport
	viewportStateCreateInfo.scissorCount  = 1;			// Numero di Scissor
	viewportStateCreateInfo.pScissors	  = &scissor;	// Puntatore ad un array di scissor

	// #4 Stage - DYNAMIC STATES
	/*
	
	// Stati dinamici da abilitare (permettere la resize senza ridefinire la pipeline)
	// N.B. Fare una resize non comporta la modifca delle extent delle tue immagini, quindi per effetuarla
	// correttamente si necessita di ricostruire una SwapChain nuova.
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // Dynamic Viewport : Permette di utilizzare il comando di resize (vkCmdSetViewport(commandBuffer, 0, 1, &viewport) ...
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);  // Dynamic Scissorr : Permette la resize da un commandbuffer vkCmdSetScissor(commandBuffer, 0, 1, &viewport) ...


	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType			 = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO; // Tipo di struttura
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());	 //
	dynamicStateCreateInfo.pDynamicStates	 = dynamicStateEnables.data();

	*/

	// #5 Stage - RASTERIZER
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType					 = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;	
	rasterizerCreateInfo.depthClampEnable		 = VK_FALSE;						// Evita il depth-clamping del far-plane, quando un oggetto va al di fuori del farplane 
																					// viene schiacciato su quest'ultimo anzichè non venir caricato.
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;						// Se abilitato scarta tutti i dati e non crea alcun fragment 
																					// viene utilizzato per pipeline dove si devono recuperare i dati (dal geometry e tess) e non disegnare niente 
	rasterizerCreateInfo.polygonMode			 = VK_POLYGON_MODE_FILL;			// Modalità di colorazione della primitiva
	rasterizerCreateInfo.lineWidth				 = 1.0f;							// Thickness delle linee delle primitive
	rasterizerCreateInfo.cullMode				 = VK_CULL_MODE_BACK_BIT;			// Quale faccia del triangolo non disegnare (la parte dietro non visibile)
	rasterizerCreateInfo.frontFace				 = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Rispetto da dove stiamo guardando il triangolo, se i punti sono stati disegnati in ordine orario allora stiamo guardano la primitiva da di fronte
	rasterizerCreateInfo.depthBiasEnable		 = VK_FALSE;						// Depth Bias tra i fragments, evita la problematica della Shadow Acne
	
	// #6 Stage - MULTISAMPLING
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.sType				   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.sampleShadingEnable  = VK_FALSE;				// Disabilita il SampleShading
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Numero di sample utilizzati per fragment

	// #7 Stage - BLENDING
	// Decide come mescolare un nuovo colore in un fragment, utilizzando il vecchio valore
	// Equazione di blending [LERP] = (srcColour*newColour)colorBlendOp(dstColourBlendFactor * oldColour)
	VkPipelineColorBlendAttachmentState colourStateAttachment = {};	
	colourStateAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | // Colori su cui applicare (write) il blending
										   VK_COLOR_COMPONENT_G_BIT |
										   VK_COLOR_COMPONENT_B_BIT |
										   VK_COLOR_COMPONENT_A_BIT;

	colourStateAttachment.blendEnable	= VK_TRUE;	// Abilita il blending

	// blending [LERP] = (VK_BLEND_FACTOR_SRC_ALPHA*newColour)colorBlendOp(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColour)
	//			[LERP] = (newColourAlpha * newColour) + ((1-newColourAlpha) * oldColour)
	colourStateAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			 // srcColor -> Componente Alpha
	colourStateAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // dstColor -> 1 - Componente Alpha
	colourStateAttachment.colorBlendOp		  = VK_BLEND_OP_ADD;					 // Operazione di blending (somma)

	// (1*newAlpha) + (0*oldAlpha) = newAlpha
	colourStateAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Moltiplica il nuovo alpha value per 1
	colourStateAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Moltiplica il vecchio alpha value per 0
	colourStateAttachment.alphaBlendOp		  = VK_BLEND_OP_ADD;	  // Potremmo utilizzare anche una sottrazione ed il risultato sarebbe il medesimo
	
	VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
	colourBlendingCreateInfo.sType			 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendingCreateInfo.logicOpEnable	 = VK_FALSE;			   // Disabilita l'alternativa di utilizzare le operazioni logiche al posto dei calcoli aritmetici
	colourBlendingCreateInfo.attachmentCount = 1;					   // Numero di attachments
	colourBlendingCreateInfo.pAttachments	 = &colourStateAttachment; // Puntatore ad un array di attachments

	/* Pipeline Layout */
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount			= 1;					  // Numero di DescriptorSet
	pipelineLayoutCreateInfo.pSetLayouts			= &m_descriptorSetLayout; // Puntatore ad un array di DescriptorSet
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;					  // Numero di PushCostants
	pipelineLayoutCreateInfo.pPushConstantRanges	= nullptr;				  // Puntatore ad un array di PushCostants

	VkResult res = vkCreatePipelineLayout(m_mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout.");
	}

	// TODO: Impostare la profondità del depth stencing test
		
	// Creazione della pipeline
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType				= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; 
	pipelineCreateInfo.stageCount			= 2;						 // Numero di Shader Stages
	pipelineCreateInfo.pStages				= shaderStages;				 // Puntatore all'array di Shader Stages
	pipelineCreateInfo.pVertexInputState	= &vertexInputCreateInfo;	 // Vertex Shader
	pipelineCreateInfo.pInputAssemblyState	= &inputAssemblyCreateInfo;	 // Input Assembly 
	pipelineCreateInfo.pViewportState		= &viewportStateCreateInfo;	 // ViewPort 
	pipelineCreateInfo.pDynamicState		= nullptr;					 // Dynamic States 
	pipelineCreateInfo.pRasterizationState	= &rasterizerCreateInfo;	 // Rasterization
	pipelineCreateInfo.pMultisampleState	= &multisampleCreateInfo;	 // Multisampling 
	pipelineCreateInfo.pColorBlendState		= &colourBlendingCreateInfo; // Colout Blending 
	pipelineCreateInfo.pDepthStencilState	= nullptr;					 // Depth Stencil
	pipelineCreateInfo.layout				= m_pipelineLayout;			 // Pipeline Layout
	pipelineCreateInfo.renderPass			= m_renderPass;				 // RenderPass Stage
	pipelineCreateInfo.subpass				= 0;						 // Subpass utilizzati nel RenderPass
	
	// Le derivate delle pipeline permettono di derivare da un altra pipeline (soluzione ottimale)
	pipelineCreateInfo.basePipelineHandle   = VK_NULL_HANDLE; // Pipeline da cui derivare
	pipelineCreateInfo.basePipelineIndex	= -1;			  // Indice della pipeline da cui derivare (in caso in cui siano passate molteplici)

	res = vkCreateGraphicsPipelines(m_mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}

	// Distruzione degli Shader Module
	vkDestroyShaderModule(m_mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

// Costruzione del RenderPass e dei SubPass
void VulkanRenderer::createRenderPass()
{
	/* Attachment del RenderPass */
	// Definizione del Layout iniziale e Layout finale del RenderPass
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.flags			= 0;								// Proprietà addizionali degli attachment
	colourAttachment.format			= m_swapChainImageFormat;			// Formato dell'immagini utilizzato nella SwapChain
	colourAttachment.samples		= VK_SAMPLE_COUNT_1_BIT;			// Numero di samples da utilizzare per l'immagine (multisampling)
	colourAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;		// Dopo aver caricato il colourAttachment performerà un operazione (pulisce l'immagine)
	colourAttachment.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;		// Al teermine dell'opperazione di RenderPass si esegue un operazione di store dei dati
	colourAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // Descrive cosa fare con lo stencil prima del rendering
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Descrive cosa fare con lo stencil dopo l'ultimo subpass, quindi dopo il rendering

	// FrameBuffer data vengono salvate come immagini, ma alle immagini è possibile 
	// fornire differenti layout dei dati, questo per motivi di ottimizzazione nell'uso di alcune operazioni.
	// Un determinato formato può essere più adatto epr una particolare operazione, per esempio uno shader
	// legge i dati in un formato particolare, ma quando presentiamo quei dati allo schermo quel formato è differente
	// perchè deve essere presentato in una certa maniera allo schermo.
	// È presente un layout intermedio che verrà svolto dai subpasses
	colourAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;		// Layout dell'immagini prima del RenderPass sarà non definito
	colourAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Layout delle immagini dopo il RenderPass, l'immagine avrà un formato presentabile

	/* SubPass */
	// Per passare l'attachment al Subpass si utilizza l'AttachmentReference.
	// È un tipo di dato che funge da riferimento/indice nella lista 'colourAttachment' definita
	// nella createInfo del RenderPass.
	VkAttachmentReference colourAttachmentReference = {};
	colourAttachmentReference.attachment = 0;										 // L'indice dell'elemento nella lista "colourAttachment" definita nel RenderPass.
	colourAttachmentReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Formato ottimale per il colorAttachment (conversione implicita UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL)

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint	 = VK_PIPELINE_BIND_POINT_GRAPHICS; // Tipo di Pipeline supportato dal Subpass (Pipeline Grafica)
	subpass.colorAttachmentCount = 1;								// Numero di ColourAttachment utilizzati.
	subpass.pColorAttachments	 = &colourAttachmentReference;		// Puntatore ad un array di colorAttachment
	
	/* SubPass Dependencies */
	std::array<VkSubpassDependency, 2> subpassDependencies;
		
	// VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	subpassDependencies[0].srcSubpass	 = VK_SUBPASS_EXTERNAL;					 // La dipendenza ha un ingresso esterno (indice)
	subpassDependencies[0].srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Indica lo stage dopo il quale accadrà la conversione di layout (dopo il termine del Subpass esterno)
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;			 // L'operazione (read) che deve essere fatta prima di effettuare la conversione.

	subpassDependencies[0].dstSubpass	 = 0;											  // La dipendenza ha come destinazione il primo SubPass della lista 'subpass'
	subpassDependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Indica lo stage prima del quale l'operazione deve essere avvenuta (Output del final color).
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |		  // Indica le operazioni prima delle quali la conversione (read e write) deve essere già avvenuta
										   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;		  // (Questi access mask indicano gli stage dove accadono)
	subpassDependencies[0].dependencyFlags = 0;											  // Disabita eventuali flags inerenti all'esecuzione e memoria

	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	subpassDependencies[1].srcSubpass	 = 0;											  // Indice del primo Subpass
	subpassDependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // La conversione deve avvenire dopo l'output del final color
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |		  // La conversione deve avvenire dopo lettura/scrittura
										   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;		 

	subpassDependencies[1].dstSubpass	   = VK_SUBPASS_EXTERNAL;				   // La dipendenza ha come destinazione l'esterno
	subpassDependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // La conversione deve avvenire prima della fine della pipeline
	subpassDependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;			   // La conversione deve avvenire prima della lettura
	subpassDependencies[1].dependencyFlags = 0;									   // Disabita eventuali flags inerenti all'esecuzione e memoria

	/* Render Pass */
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;		  
	renderPassCreateInfo.attachmentCount = 1;												  // Numero di attachment presenti nel Renderpass
	renderPassCreateInfo.pAttachments	 = &colourAttachment;								  // Puntatore all'array di attachments
	renderPassCreateInfo.subpassCount	 = 1;												  // Numero di Subpasses coinvolti
	renderPassCreateInfo.pSubpasses		 = &subpass;										  // Puntatore all'array di Subpass
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size()); // Numero di Subpass Dependencies coinvolte
	renderPassCreateInfo.pDependencies	 = subpassDependencies.data();						  // Puntatore all'array/vector di Subpass Dependencies

	VkResult res = vkCreateRenderPass(m_mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass);
	
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Render Pass!");
	}
}

// Creazione dei Framebuffer, effettuano una connessione tra il RenderPass e le immagini. Utilizzando rispettivamente Attachments ed ImageView
void VulkanRenderer::createFramebuffers()
{
	m_swapChainFrameBuffers.resize(m_swapChainImages.size());

	// Creiamo un framebuffer per ogni immagine della swapchain
	for (size_t i = 0; i < m_swapChainFrameBuffers.size(); ++i)
	{
		std::array<VkImageView, 1> attachments = {
			m_swapChainImages[i].imageView
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType			  = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass	  = m_renderPass;							   // RenderPass
		frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // Numero degli attachments
		frameBufferCreateInfo.pAttachments	  = attachments.data();						   // Lista degli attachments 1:1 con il RenderPass
		frameBufferCreateInfo.width			  = m_swapChainExtent.width;				   // Framebuffer width
		frameBufferCreateInfo.height		  = m_swapChainExtent.height;				   // Framebuffer height
		frameBufferCreateInfo.layers		  = 1;										   // Framebuffer layers

		VkResult result = vkCreateFramebuffer(m_mainDevice.logicalDevice, &frameBufferCreateInfo, nullptr, &m_swapChainFrameBuffers[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Framebuffer!");
		}

	}
}

// Creazione di una Command Pool (permette di allocare i Command Buffers)
void VulkanRenderer::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType			  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily;		// I comandi dei CommandBuffers funzionano solo per le Graphic Queues
	
	VkResult res = vkCreateCommandPool(m_mainDevice.logicalDevice, &poolInfo, nullptr, &m_graphicsComandPool);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Command Pool!");
	}
}

// Allocazione dei Command Buffers (gruppi di comandi da inviare alle queues)
void VulkanRenderer::createCommandBuffers()
{
	// Tanti CommandBuffers quanti FramBuffers
	m_commandBuffer.resize(m_swapChainFrameBuffers.size());

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool	= m_graphicsComandPool;				// Command Pool dalla quale allocare il Command Buffer
	cbAllocInfo.level		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// Livello del Command Buffer
																// VK_COMMAND_BUFFER_LEVEL_PRIMARY   : Il CommandBuffer viene inviato direttamente sulla queue. 
																// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Il CommandBuffer non può essere chiamato direttamente, ma da altri buffers via "vkCmdExecuteCommands".
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffer.size()); // Numero di CommandBuffers da allocare

	VkResult res = vkAllocateCommandBuffers(m_mainDevice.logicalDevice, &cbAllocInfo, m_commandBuffer.data());
	
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Command Buffers!");
	}
}

// Sincronizzazione : 1 semaforo per le immagini disponibili, 1 semaforo per le immagini presentabili/renderizzate, 1 fence per l'operazione di draw
void VulkanRenderer::createSynchronisation()
{
	m_syncObjects.resize(MAX_FRAMES_IN_FLIGHT);

	//m_renderFinished.resize(MAX_FRAMES_IN_FLIGHT);
	//m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);


	// Semaphore
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;		// Specifica che viene creato nello stato di SIGNALED

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(m_mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &m_syncObjects[i].m_imageAvailable) != VK_SUCCESS ||
			vkCreateSemaphore(m_mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &m_syncObjects[i].m_renderFinished) != VK_SUCCESS ||
			vkCreateFence(m_mainDevice.logicalDevice,	  &fenceCreateInfo,		nullptr, &m_syncObjects[i].m_inFlight) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphores and/or Fence!");
		}
	}
}

void VulkanRenderer::createDescriptorSetLayout()
{
	// Informazioni per il binding della struct MVP
	VkDescriptorSetLayoutBinding mvpLayoutBinding = {};
	mvpLayoutBinding.binding		    = 0;								 // Punto di binding nello shader
	mvpLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // UBO
	mvpLayoutBinding.descriptorCount    = 1;								 // Numero di descriptors di bindare
	mvpLayoutBinding.stageFlags		    = VK_SHADER_STAGE_VERTEX_BIT;		 // Shaders nel quale bindare
	mvpLayoutBinding.pImmutableSamplers = nullptr;							 // rendere il sampler immutable specificando il layout, serve per le textures

	// Crea il layout per il Descriptor Set
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = 1;					// Numero di binding infos
	layoutCreateInfo.pBindings    = &mvpLayoutBinding;	// Array di binding infos
	
	// Creazione del layout del descriptor set
	VkResult res = vkCreateDescriptorSetLayout(m_mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &m_descriptorSetLayout);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor set layout");
	}
}

void VulkanRenderer::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(MVP);

	// Un UBO per ogni immagine
	m_uniformBuffer.resize(m_swapChainImages.size());
	m_uniformBufferMemory.resize(m_swapChainImages.size());

	for (size_t i = 0; i < m_swapChainImages.size(); i++)
	{
		createBuffer(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, bufferSize, 
					 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					 &m_uniformBuffer[i], &m_uniformBufferMemory[i]);
	}
}

void VulkanRenderer::createDescriptorPool()
{
	// Tipi di descriptors (no descriptor set) e quanti
	VkDescriptorPoolSize poolSize = {};
	poolSize.type			 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;			   // Che tipo di descriptor contiene (non è un descriptor set ma sono quelli presenti negli shaders)
	poolSize.descriptorCount = static_cast<uint32_t> (m_uniformBuffer.size()); // Quanti descriptor

	// Data per la creazione della pool
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType		 = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets		 = static_cast<uint32_t>(m_uniformBuffer.size()); // un descriptor set per ogni commandbuffer/swapimage
	poolCreateInfo.poolSizeCount = 1;											  // Quante pool
	poolCreateInfo.pPoolSizes	 = &poolSize;									  // Array di pool size

	// Creazione della descriptor pool
	VkResult res = vkCreateDescriptorPool(m_mainDevice.logicalDevice, &poolCreateInfo, nullptr, &m_descriptorPool);
	
	if(res!= VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}
}

void VulkanRenderer::createDescriptorSets()
{
	// Un descriptor set per ogni buffer (o immagine)
	m_descriptorSets.resize(m_uniformBuffer.size());

	// Lista di tutti i possibili layour che useremo dal set (?) non capito TODO
	std::vector<VkDescriptorSetLayout> setLayouts(m_uniformBuffer.size(), m_descriptorSetLayout);

	// Informazioni per l'allocazione del descriptor set
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool		= m_descriptorPool;								 // Pool per l'allocazione dei descriptor set
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_uniformBuffer.size()); // 
	setAllocInfo.pSetLayouts		= setLayouts.data();							 // Layout da utilizzare per allocare i set (1:1)

	// Allocazione dei descriptor sets (molteplici)
	VkResult res = vkAllocateDescriptorSets(m_mainDevice.logicalDevice, &setAllocInfo, m_descriptorSets.data());

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Descriptor Sets!");
	}

	// Aggiornamento dei bindings dei descriptor sets
	for (size_t i = 0; i < m_uniformBuffer.size(); ++i)
	{
		// Descrizione del buffer che andremo a bindare

		VkDescriptorBufferInfo mvpBufferInfo = {};
		mvpBufferInfo.buffer = m_uniformBuffer[i]; // Buffer da cui prendere i dati
		mvpBufferInfo.offset = 0;				   // Offset da cui partire
		mvpBufferInfo.range  = sizeof(MVP);		   // Dimensione dei dati

		VkWriteDescriptorSet mvpSetWrite = {}; // DescriptorSet per MVP
		mvpSetWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		mvpSetWrite.dstSet			= m_descriptorSets[i];
		mvpSetWrite.dstBinding		= 0;										// Vogliamo aggiornare quello che binda su 0
		mvpSetWrite.dstArrayElement = 0;										// Se avessimo un array questo
		mvpSetWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// Indice nell'array da aggiornare
		mvpSetWrite.descriptorCount = 1;										// Numero di descriptor da aggiornare
		mvpSetWrite.pBufferInfo		= &mvpBufferInfo;							// Informazioni a riguardo dei dati del buffer da bindare
	
		// Aggiornamento dei descriptor sets con i nuovi dati del buffer/binding
		vkUpdateDescriptorSets(m_mainDevice.logicalDevice, 1, &mvpSetWrite, 0, nullptr);
	}
}

// Copia dei dati MVP della CPU nell'UniformBufferObject della GPU
void VulkanRenderer::updateUniformFunctions(uint32_t imageIndex)
{
	void* data;
	// Mappa l'UniformBufferObject nello spazio di indirizzamento dell'applicazione, salvando nel puntatore 'data' l'intervallo mappato sulla memoria.
	vkMapMemory(m_mainDevice.logicalDevice, m_uniformBufferMemory[imageIndex], 0, sizeof(MVP), 0, &data);

	// Copio i dati della CPU sulla GPU
	memcpy(data, &m_mvp, sizeof(MVP));

	// Elimino il mapping
	vkUnmapMemory(m_mainDevice.logicalDevice, m_uniformBufferMemory[imageIndex]);
}

// Costruzione di uno Shader Module a partire dal codice intermedio SPIR-V
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;	  
	shaderModuleCreateInfo.codeSize = code.size();									  // Dimensione del codice SPIR-V
	shaderModuleCreateInfo.pCode	= reinterpret_cast<const uint32_t*>(code.data()); // Puntatore al codice SPIR-V

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(m_mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Shader Module!");
	}

	return shaderModule;
}

VulkanRenderer::~VulkanRenderer()
{
	cleanup();
}

void VulkanRenderer::cleanup()
{
	// Aspetta finchè nessun azione sia eseguita sul device senza distruggere niente
	// Tutte le operazioni effettuate all'interno della draw() sono in asincrono.
	// Questo significa che all'uscita del loop della init(), le operazionio di drawing
	// e di presentazione potrebbero ancora essere in corso ed eliminare le risorse mentre esse sono in corso è una pessima idea
	// quindi è corretto aspettare che il dispositivo sia inattivo prima di eliminare gli oggetti.
	vkDeviceWaitIdle(m_mainDevice.logicalDevice);

	// Distrugge della Pool di DescriptorSet
	vkDestroyDescriptorPool(m_mainDevice.logicalDevice, m_descriptorPool, nullptr);

	// Distrugge del layout del DescriptorSet
	vkDestroyDescriptorSetLayout(m_mainDevice.logicalDevice, m_descriptorSetLayout, nullptr);

	// Distrugge i buffers per i DescriptorSet
	for (size_t i = 0; i < m_uniformBuffer.size(); i++)
	{
		vkDestroyBuffer(m_mainDevice.logicalDevice, m_uniformBuffer[i], nullptr);
		vkFreeMemory(m_mainDevice.logicalDevice, m_uniformBufferMemory[i], nullptr);
	}

	// Distrugge le Meshes
	for (size_t i = 0; i < m_meshList.size(); i++)
	{
		m_meshList[i].destroyBuffers();
	}

	// Distrugge i Semafori
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_mainDevice.logicalDevice, m_syncObjects[i].m_renderFinished, nullptr);
		vkDestroySemaphore(m_mainDevice.logicalDevice, m_syncObjects[i].m_imageAvailable, nullptr);
		vkDestroyFence(m_mainDevice.logicalDevice, m_syncObjects[i].m_inFlight, nullptr);
	}

	// Distrugge la Command Pool
	vkDestroyCommandPool(m_mainDevice.logicalDevice, m_graphicsComandPool, nullptr);

	// Distrugge tutti i Framebuffers
	for (auto framebuffer : m_swapChainFrameBuffers)
	{
		vkDestroyFramebuffer(m_mainDevice.logicalDevice, framebuffer, nullptr);
	}

	// Distrugge la Pipeline Grafica
	vkDestroyPipeline(m_mainDevice.logicalDevice, m_graphicsPipeline, nullptr);

	// Distrugge la Pipeline Layout
	vkDestroyPipelineLayout(m_mainDevice.logicalDevice, m_pipelineLayout, nullptr);

	// Distrugge il RenderPass
	vkDestroyRenderPass(m_mainDevice.logicalDevice, m_renderPass, nullptr);

	// Distrugge le immagini nella SwapChain
	for (auto image : m_swapChainImages)
	{
		vkDestroyImageView(m_mainDevice.logicalDevice, image.imageView, nullptr);
	}

	// Distrugge la SwapChain
	vkDestroySwapchainKHR(m_mainDevice.logicalDevice, m_swapchain, nullptr);

	// Distrugge la Surface
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);	// Distrugge la Surface (GLFW si utilizza solo per settarla)

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	// Distrugge il device logico
	vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);
	
	// Distrugge l'istanza di Vulkan
	vkDestroyInstance(m_instance, nullptr);					
}
