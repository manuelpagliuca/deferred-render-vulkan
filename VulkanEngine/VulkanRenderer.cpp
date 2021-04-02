#include "VulkanRenderer.h"

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
		createRenderPass();
		createGraphicPipeline();
		createFramebuffers();
		createCommandPool();

		// Creazione della mesh
		std::vector<Vertex> meshVertices = {
			{ { -0.1, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
			{ { -0.1, 0.4, 0.0 },{ 0.0f, 1.0f, 0.0f } },	// 1
			{ { -0.9, 0.4, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
			{ { -0.9, -0.4, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
		};

		std::vector<Vertex> meshVertices2 = {
			{ { 0.9, -0.3, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
			{ { 0.9, 0.1, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
			{ { 0.1, 0.3, 0.0 },{ 0.0f, 0.0f, 1.0f } },     // 2
			{ { 0.1, -0.3, 0.0 },{ 1.0f, 1.0f, 0.0f } },    // 3
		};

		// Index Data
		std::vector<uint32_t> meshIndices = {
			0, 1, 2,
			2, 3, 0
		};
		
		firstMesh = Mesh(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, 
						 m_graphicsQueue, m_graphicsComandPool, 
						 &meshVertices);

		createCommandBuffers();
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

// Si occupa di disegnare le immagini a schermo
// 2 semafori per avvisare quando l'immagine è pronta e d uno per avvisare quanto l'immagine è pronta per essere presentata a schermo
void VulkanRenderer::draw()
{
	// Aspettare per il dato Fence il segnale dell'ultimo draw effettauto prima di continuare
	vkWaitForFences(m_mainDevice.logicalDevice, 1, &m_drawFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	// Manualmente resetta le fences
	vkResetFences(m_mainDevice.logicalDevice, 1, &m_drawFences[m_currentFrame]);
		
	// 1. Prende la prossima immagine disposnibile e imposta un segnale che avvisa quando l'immagine ha terminato (semaphore)

	// Preleva l'index della prossima immagine da mostrare, quando pronta manda un segnale al semaforo
	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_mainDevice.logicalDevice, m_swapchain, std::numeric_limits<uint64_t>::max(), m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	// -- Invia il command buffer al render -- 
	// Informazioni per l'invio alla queue
	// Il renderer non disegna finchè image available non è impostata a true
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;				// Tipo di struttura
	submitInfo.waitSemaphoreCount = 1;								// Numero dei semafori da aspettare
	submitInfo.pWaitSemaphores = &m_imageAvailable[m_currentFrame];	// Lista dei semafori da aspettare

	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	// Stages dove controllare il semaforo
	};
		
	submitInfo.pWaitDstStageMask	= waitStages;
	submitInfo.commandBufferCount	= 1;								 // Numero di command buffer da inviare
	submitInfo.pCommandBuffers		= &m_commandBuffer[imageIndex];		 // Command buffer to submit
	submitInfo.signalSemaphoreCount = 1;								 // Numero di semafori a cui segnalare
	submitInfo.pSignalSemaphores	= &m_renderFinished[m_currentFrame]; // Semaforo a cui viene inviato il segnale quando il render è terminato

	// Invia il command buffer alla queue, quando finisce di disegnare manda un segnale al relativo Fence del frame
	VkResult result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_drawFences[m_currentFrame]);
	
	if (result != VK_SUCCESS)
	{
		std::runtime_error("Failed to submit command buffer to queue!");
	}

	// -- PRESENTA L'IMMAGINE RENDERIZZATA A SCHERMO --
	VkPresentInfoKHR presentInfo   = { };
	presentInfo.sType			   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;// Tipo di struttura
	presentInfo.waitSemaphoreCount = 1;									// Numero di semafori da aspettare prima di renderizzare
	presentInfo.pWaitSemaphores	   = &m_renderFinished[m_currentFrame]; // Semaforo relativo al termine del rendering
	presentInfo.swapchainCount	   = 1;									// Numero di swapchains a cui presentare
	presentInfo.pSwapchains		   = &m_swapchain;	 					// Swapchain contenente le immagini
	presentInfo.pImageIndices	   = &imageIndex;						// Indice dell'immagine nella swapchain da visualizzare

	// Presentazione dell'immagine
	result = vkQueuePresentKHR(m_presentationQueue, &presentInfo);

	if (result != VK_SUCCESS)
	{
		std::runtime_error("Failed to present the image!");
	}

	// Prende il prossimo frame
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::createInstance()
{
	// APPLICATION INFO (per la creazione di un istanza di Vulkan si necessitano le informazioni a riguardo dell'applicazione)
	VkApplicationInfo appInfo = {};

	appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO; // Tipo di struttura
	appInfo.pApplicationName	= "Vulkan Render Application";		  // Nome dell'applicazione
	appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);			  // Versione personalizzata dell'applicazione
	appInfo.pEngineName			= "2148_RENDERER";					  // Nome dell'engine
	appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);			  // Versione personalizzata dell'engine
	appInfo.apiVersion			= VK_API_VERSION_1_0;				  // Versione di Vulkan che si vuole utilizzare (unico obbligatorio)

	// VULKAN INSTANCE
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	// Tipo di struttura
	createInfo.pApplicationInfo		= &appInfo;									// Puntatore alle informazioni dell'applicazione

	std::vector <const char*> instanceExtensions = std::vector<const char*>();		// Vettore che conterrà le estensioni (come cstring)
	loadGlfwExtensions(instanceExtensions);											// Preleva le estensioni di GLFW per Vulkan e le carica nel vettore
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);				// Carica nel vettore l'estensione che permette di creare un debug messanger (e altro...)

	if (!checkInstanceExtensionSupport(&instanceExtensions))								// Controlla se Vulkan supporta le estensioni presenti in instanceExtensions
	{
		throw std::runtime_error("VkInstance doesn't support the required extensions");		// Nel caso in cui le estensioni non siano supportate throwa un errore runtime
	}
	
	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };				// Vettore contenente le Validation Layers (come cstring)

	if (enableValidationLayers && !checkValidationLayerSupport(&validationLayers))				// Controlla che le Validation Layers richieste siano effettivamente supportate
	{																							// (nel caso in cui siano state abilitate).
		throw std::runtime_error("VkInstance doesn't support the required validation layers");	// Nel caso in cui le Validation Layers non siano disponibili
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()); // Numero di estensioni abilitate
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();						 // Prende le estensioni abilitate (e supportate da Vulkan)
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());		 // Numero di Validation Layers abilitate.
	createInfo.ppEnabledLayerNames = validationLayers.data();							 // Prende le Validation Layers abilitate (e supportate da Vulkan)
	VkResult res = vkCreateInstance(&createInfo, nullptr, &m_instance);					 // Crea un istanza di Vulkan.

	if (res != VK_SUCCESS) // Nel caso in cui l'istanza non venga creata correttamente, alza un eccezione runtime.
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}
}

// Scarica le estensioni relative a GLFW
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
	// Informazioni sul come iniziare ogni command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;  //
	
	// Informazioni sul come iniziare il Render Pass (necessario solo per le applicazioni grafiche)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType			  = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; // Tipo di struttura 
	renderPassBeginInfo.renderPass		  = m_renderPass;							  //
	renderPassBeginInfo.renderArea.offset = { 0,0 };								  // Start point del RenderPass (in Pixels)
	renderPassBeginInfo.renderArea.extent = m_swapChainExtent;						  // Dimensione delal regione dove eseguire il render pass (partendo dall'offset)

	// Valori che vengono utilizzati per pulire l'immagine
	VkClearValue clearValues[] = {
		{0.6f, 0.65f, 0.4f, 1.0f}
	};
	renderPassBeginInfo.pClearValues	  = clearValues; // Lista dei clear values (TODO: Depth Attachment)
	renderPassBeginInfo.clearValueCount   = 1;			 // Un solo clear values utilizzato

	for (size_t i = 0; i < m_commandBuffer.size(); ++i)
	{
		// Passiamo un framebuffer per volta
		renderPassBeginInfo.framebuffer = m_swapChainFrameBuffers[i];

		// Inizia a registrare i comandi nel command buffer
		VkResult res = vkBeginCommandBuffer(m_commandBuffer[i], &bufferBeginInfo);
		
		if (res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to start recording a Command Buffer!");
		}

			// Inizio del Render Pass (VK_SUBPASS_CONTENTS_INLINE: tutti i comandi saranno primary cmd)
			vkCmdBeginRenderPass(m_commandBuffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				// Binding della Pipeline Grafica (VK_PIPELINE_BIND_POINT_GRAPHICS) da utilizzare nel Render Pass
				// È possibile impostare molteplici pipeline e switchare(per esempio una versione DEFERREd o WIREFRAME)
				vkCmdBindPipeline(m_commandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
				
				VkBuffer vertexBuffers[] = { firstMesh.getVertexBuffer() };		// Buffer da bindare prima di essere disegnati
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(m_commandBuffer[i], 0, 1, vertexBuffers, offsets);

				vkCmdDraw(m_commandBuffer[i],static_cast<uint32_t>(firstMesh.getVertexCount()), 1, 0, 0);

			// Fine del Render Pass
			vkCmdEndRenderPass(m_commandBuffer[i]);

		// Smette di registrare i comandi
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

	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr); // Prendiamo la quantità dei dispositivi fisici rilevati da Vulkan

	if (deviceCount == 0)										   // Nel caso in cui non vengano rilevati dispositivi fisici, alzo un eccezione a runtime
	{
		throw std::runtime_error("Can't find GPU that support Vulkan Instance");
	}

	std::vector<VkPhysicalDevice> deviceList(deviceCount);					 // Dimensiono un vettore per contenere i dispositivi fisici
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, deviceList.data()); // Scarica i dispositivi fisici all'interno del vettore

	for (const auto& device : deviceList)			// Controllo tra i device che si trovano all'interno della deviceList
	{												// e mi fermo al primo dispositivo che è adatto per l'applicazione
													// (ovvero che contiene delle Queue Families per la grafica)
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
	setQueueFamiliesIndices(device);	// Impostiamo gli indici delle Queue Families

	bool const extensionSupported = checkDeviceExtensionSupport(device);	// Controlla che le estensioni richieste siano disponibili nel dispositivo fisico
	bool swapChainValid = false;

	if (extensionSupported)	// Se le estensioni sono supportate (quindi anche la Surface) allora posso
	{						// controllare se la SwapChain è valida
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
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);	// Prelevo il numero di estensioni supportate nel dispositivo fisico

	if (extensionCount == 0)															// Se non sono presenti estensioni mi fermo
		return false;

	std::vector<VkExtensionProperties> extensions(extensionCount);						// Nel caso contrario le salvo all'interno del vettore
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	for (const auto& deviceExtensions : deviceExtensions)				// Controllo se le estensioni richieste sono presenti tra
	{																	// quelle prelevate dal dispositivo fisico
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(deviceExtensions, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)	// Se non sono state trovate le estensioni richieste
			return false;	// restituisco false
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

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	for(const auto &presentationMode : presentationModes)	// Cerca se la modalità MailBox è disponibile
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR; // Fa parte della specifica di Vulkan ed è una modalità SEMPRE presente
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
	deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());	// Numero di estensioni da utilizzare sul dispositivo logico.
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();							// Puntatore ad un array che contiene le estensioni abilitate (SwapChain, ...).
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

// Crea una Surface, dice a Vulkan come interfacciarsi con mia finestra (Platform Agnostic)
void VulkanRenderer::createSurface()
{
	VkResult res = glfwCreateWindowSurface(	// GLFW restituisce una Surface per Vulkan evita la costruzione
		m_instance,							// manuale (info struct) e chiama la funzione direttamente restituendone il risultato.
		m_window, 
		nullptr, 
		&m_surface);	
																		
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create the surface!");
	}

}

// Crea la Swap Chain
void VulkanRenderer::createSwapChain()
{
	// Preleva le informazioni della SwapChain (Capabilities, Formats, Presentation modes)
	SwapChainDetails swapChainDetails = getSwapChainDetails(m_mainDevice.physicalDevice); 

	// Imposto i valori ottimali della Surface in preparazione della SwapChain
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode	 = chooseBestPresentationMode(swapChainDetails.presentationModes);
	VkExtent2D extent				 = chooseSwapExtent(swapChainDetails.surfaceCapabilities);
	
	// Numero di immagini presenti nella SwapChain
	// Serve 1 in più della minima quantità di immagini per utilizzare il triple buffering
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

	// Se il numero max di immagini è pari a 0, significa non sono presenti limiti (e non che non ci sono immagini)
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && 
		swapChainDetails.surfaceCapabilities.maxImageCount < imageCount) // Se il massimo numero di immagini è minore delle immagini per il triple buffering
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount; // Allora prendiamo utilizziamo il massimo numero di immagini (significa che il triple-buffering non è disponibile)
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType					 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;			  // Tipo di struttura dati
	swapChainCreateInfo.surface					 = m_surface;											  // Surface
	swapChainCreateInfo.imageFormat				 = surfaceFormat.format;								  // Formato della SwapChain
	swapChainCreateInfo.imageColorSpace			 = surfaceFormat.colorSpace;							  // Spazio colore della SwapChain
	swapChainCreateInfo.presentMode				 = presentMode;											  // Presentation Mode della SwapChain
	swapChainCreateInfo.imageExtent				 = extent;												  // Extent della SwapChain 
	swapChainCreateInfo.minImageCount			 = imageCount;											  // Minimo numero di immagini che la Swap Chain può utilizzare
	swapChainCreateInfo.imageArrayLayers		 = 1;													  // Numero di livelli per ogni image chain
	swapChainCreateInfo.imageUsage				 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;					  // Specifica l'utilizzo dell'immagine, in questo caso che possa essere utilizzata per creare ImageView adatte per i colori
	swapChainCreateInfo.preTransform			 = swapChainDetails.surfaceCapabilities.currentTransform; // Corrente trasformazione matematica della Surface
	swapChainCreateInfo.compositeAlpha			 = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;					  // Come vengono gestite le immagini trasparenti con le grafiche esterne (2 finestre che si sovrappongono)
	swapChainCreateInfo.clipped					 = VK_TRUE;												  // 

	// Se le famiglie Graphics e Presentation sono diverse, allora la SwapChain deve lasciare le immagini
	// in maniera condivise (e concorrente) tra le due queues (non è la scelta più efficente).
	if (m_queueFamilyIndices.graphicsFamily != m_queueFamilyIndices.presentationFamily)
	{
		uint32_t queueFamilyIndices[]{
			static_cast<int32_t>(m_queueFamilyIndices.graphicsFamily),
			static_cast<int32_t>(m_queueFamilyIndices.presentationFamily)
		};

		swapChainCreateInfo.imageSharingMode	  = VK_SHARING_MODE_CONCURRENT;	// Modalità concorrente
		swapChainCreateInfo.queueFamilyIndexCount = 2;							// Numero di queue utilizzate per la condivisione delle immagini
		swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;			// Array delle contenente i riferimenti alle Queue
	}
	else
	{
		swapChainCreateInfo.imageSharingMode	  = VK_SHARING_MODE_EXCLUSIVE;	// Modalità esclusiva ad una sola Queue
		swapChainCreateInfo.queueFamilyIndexCount = 1;							// Numero di queue utilizzate per la condivisione delle immagini
		swapChainCreateInfo.pQueueFamilyIndices   = nullptr;					// Array delle contenente i riferimenti alle Queue
	}

	// Se la vecchia SwapChain viene sostituita dall'attuale, è possibile fornire un link per trasferirne la responsabilità
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;


	// Creazione della SwapChain e memorizzazione nel riferimento 'm_swapchain'
	VkResult res = vkCreateSwapchainKHR(m_mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &m_swapchain);
	
	// Si controlla che la creazione abbia avuto successo
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a SwapChain.");
	}

	// Salviamo i riferimenti relativi al formato e all'extent in vista di un utilizzo successivo
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent	   = extent;

	// Preleviamo il numero di immagini presenti nella SwapChain
	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapchain, &swapChainImageCount, nullptr);

	// Salviamo le immagini all'interno del vettore
	std::vector <VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapchain, &swapChainImageCount, images.data());

	// Iteriamo sulle immagini della SwapChain e le salviamo in un vettore apparte assieme alla relativa ImageView (creata)
	for (VkImage image : images)
	{
		SwapChainImage swapChainImage = {};

		// Memorizziamo l'mmagine da gestire nella SwapChain
		swapChainImage.image = image;

		// Creazione e memorizzazione dell'ImageView nella SwapChain
		swapChainImage.imageView = createImageView(image, m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		
		// Aggiunge alla lista delle immagini per la SwapChain
		m_swapChainImages.push_back(swapChainImage);
	}
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspetFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; // Tipo di struttura dati
	viewCreateInfo.image		= image;									// Immagine per la quale costruire la ImageView
	viewCreateInfo.viewType		= VK_IMAGE_VIEW_TYPE_2D;					// Tipo delle immagini
	viewCreateInfo.format		= format;									// Formato delle immagini
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;			// Imposto lo swizzle con il valore della componente effettiva
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;			// Imposto lo swizzle con il valore della componente effettiva
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;			// Imposto lo swizzle con il valore della componente effettiva
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;			// Imposto lo swizzle con il valore della componente effettiva

	// Le subresources permettono all'ImageView di visualizzare una sola parte dell'immagine
	viewCreateInfo.subresourceRange.aspectMask     = aspetFlags; // Quale aspetto dell'immagine visualizzare (e.g. COLOR_BIT, STENCIL, ...)
	viewCreateInfo.subresourceRange.baseMipLevel   = 0;			 // Livello iniziale della mipmap da visualizzare
	viewCreateInfo.subresourceRange.levelCount     = 1;			 // Numero di livelli mipmap 
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;			 // Livello iniziale del primo ArrayLayer
	viewCreateInfo.subresourceRange.layerCount	   = 1;			 // Numero di ArrayLayer

	// Creazione della ImageView
	VkImageView imageView;
	VkResult res = vkCreateImageView(m_mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
	
	// Controlliamo se l'ImageView è stata creata con successo
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image View!");
	}

	// In tal caso la restituiamo
	return imageView;
}

// Imposta gli indici della Queue Families nella data structure interna
void VulkanRenderer::setQueueFamiliesIndices(VkPhysicalDevice device)
{
	uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);				 // Prelevo la quantità di Queue Families disponibili sul device fisico	
	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);						 // Creo un vettore della dimensione appropriata
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data()); // Inserisco le Queue Families nel vettore

	// È responsabilità del programmatore tracciare l'indice delle Queue Families, non sono presenti indici a riguardo nelle strutture dati di Vulkan.
	int queueIndex = 0;

	for (const auto& queueFamily : queueFamilyList)
	{
		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))		// Se sono presenti Queue Families e queste Queue Families fanno parte della
		{																						// famiglia QUEUE_GRAPHICS (queue per la grafica), allora ne salvo l'indice all'interno.
			m_queueFamilyIndices.graphicsFamily = queueIndex;
		}

		VkBool32 presentationSupport = false;

		vkGetPhysicalDeviceSurfaceSupportKHR( // Se nella Surface di un Device Fisico è presente una Queue Family che supporta la
			device,							  // presentation mode, allora restituisci un bool in "presentationSupport"
			queueIndex,					
			m_surface,
			&presentationSupport);		

		if (queueFamilyCount > 0 && presentationSupport)			// Se esiste almeno una Queue Family che supporta la presentation mode
		{
			m_queueFamilyIndices.presentationFamily = queueIndex;	// allora salvo l'indice di quella Queue Family
		}

		if (m_queueFamilyIndices.isValid())		// Se gli indici delle Queue Families sono tutti validi, fermiamoci
		{
			break;
		}

		++queueIndex;
	}
}

// Restituisce le informazioni riguardanti la SwapChain (Surface Capabilities, Formati supportati, Presentation Modes supportate)
SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapChainDetails;

	/* Surface Capabilities */
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(	// Salva le Surface Capabilities all'interno 
		device,									// della swapChainDetails.surfaceCapabilities
		m_surface,
		&swapChainDetails.surfaceCapabilities);

	/* Formati immagine supportati dalla Surface */
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(		// Preleva il numero di image formats supportati dalla Surface
		device,									 
		m_surface,
		&formatCount,
		nullptr);

	if (formatCount > 0)								// Se il numero di formati supportati è positivo
	{													// vengono salvate all'interno del vettore 'formats' presente nella struttura dati
		swapChainDetails.formats.resize(formatCount);

		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device,
			m_surface,
			&formatCount,
			swapChainDetails.formats.data()
		);
	}

	/* Modalità di presentazione supportate dalla Surface */
	uint32_t presentationCount = 0;				 
	vkGetPhysicalDeviceSurfacePresentModesKHR(	// Preleva il numero di modalità di presentazione supportate dalla surface 
		device,
		m_surface,
		&presentationCount,
		nullptr
	);

	if (presentationCount > 0)											// Se il numero di modalità è positivo
	{
		swapChainDetails.presentationModes.resize(presentationCount);	// Vengono caricate all'interno del vettore 'presentationModes' presente all'interno della struttura dati

		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device,
			m_surface,
			&presentationCount,
			swapChainDetails.presentationModes.data()
		);
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
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |			// Imposta tutti i tipi di severità a cui siamo interessati
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType	   = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT	|			// Imposta il tipo di messaggio disponibili
								 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = debugCallback;	// Imposta la funzione di callback da chiamare in caso di errore
	createInfo.pUserData	   = nullptr;  		// Dati extra da passare all'interno della callback

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) // Creo il debug messanger, in caso di errore alzo un eccezione
	{
		throw std::runtime_error("Failed to set up debug messenger!");
	}
}

// Callback, formattazione del testo in base al tipo di messaggio
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT &&
		messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
	{
		std::cerr << "[Validation Layer][Error][Violation]: "<< pCallbackData->pMessage << "\n\n";
	}
	/* Set levels of Validation Layer
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		std::cerr << " [Warning]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		std::cerr << " [Verbose]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		std::cerr << " [Info]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		std::cerr << " [Error]";
		break;
	}

	switch (messageType)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		std::cerr << "-[General]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		std::cerr << "-[Violation]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		std::cerr << "-[Non-Optimal]";
		break;
	}*/

	return VK_FALSE;
}

// Crea il debug messanger
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

// Distrugge il debug messanger
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
	// Lettura dei codici intermedi SPIR-V
	std::vector<char> vertexShaderCode	 = readFile("./shaders/vert.spv");
	std::vector<char> fragmentShaderCode = readFile("./shaders/frag.spv");

	// Costruzione dei moduli shader
	VkShaderModule vertexShaderModule	 = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule  = createShaderModule(fragmentShaderCode);

	// -- INFO PER GLI SHADER STAGE --

	// Vertex Stage createinfo
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType						   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; // Tipo di struttura dati
	vertexShaderCreateInfo.stage						   = VK_SHADER_STAGE_VERTEX_BIT;						  // Nome dello Shader Stage
	vertexShaderCreateInfo.module						   = vertexShaderModule;								  // Modulo da utilizzare
	vertexShaderCreateInfo.pName						   = "main";											  // Entry point dello shader

	// Fragment Stage createinfo
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType							 = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; // Tipo di struttura dati
	fragmentShaderCreateInfo.stage							 = VK_SHADER_STAGE_FRAGMENT_BIT;						  // Nome dello Shader Stage
	fragmentShaderCreateInfo.module							 = fragmentShaderModule;								  // Modulo da utilizzare
	fragmentShaderCreateInfo.pName							 = "main";											  // Entry point dello shader

	// Vettore di ShaderStage (la pipeline grafica richiede un vettore di shader)
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// -- PIPELINE --

	// Descrizione dei dati per un singolo vertice (posizione, colori, tex-coords, normals, ...)
	VkVertexInputBindingDescription bindingDescription = {}; // Non è una createInfo quindi non ha un sType
	bindingDescription.binding   = 0;							// Può bindarsi a molteplici streams di dati
	bindingDescription.stride    = sizeof(Vertex);				// Dimensione di un singolo vertice
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Come muoversi tra i dati dopo ogni vertice
																// VK_VERTEX_INPUT_RATE_VERTEX : Si sposta al prossimo vertice
																// VK_VERTEX_INPUT_RATE_INSTANCE : Si sposta al prossimo vertice per la prossima istanza
																
	// Come i dati per un attributo sono definiti in un vertice
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;
	
	// Attributo per la posizione
	attributeDescriptions[0].binding  = 0;						    // Su quale binding di dati si trova (deve essere lo stesso di sopra)
	attributeDescriptions[0].location = 0;						    // Location di binding nel Vertex Shader
	attributeDescriptions[0].format	  = VK_FORMAT_R32G32B32_SFLOAT; // Formato assunto dei dati + la dimensione
	attributeDescriptions[0].offset	  = offsetof(Vertex, pos);		// Dove l'attributo è definito nei dati per un singolo vertice

	// Attributo per il colore
	attributeDescriptions[1].binding = 0;						  
	attributeDescriptions[1].location = 1;						  
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; 
	attributeDescriptions[1].offset = offsetof(Vertex, col);		


	// #1 STAGE - VERTEX INPUT
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount		= 1;	// 
	vertexInputCreateInfo.pVertexBindingDescriptions		= &bindingDescription; // Lista delle descrizioni di binding dei vertici (stride/spacing infos)
	vertexInputCreateInfo.vertexAttributeDescriptionCount	= static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions		= attributeDescriptions.data(); // Lista delle descrizione degli attributi dei vertici (formato dati, dove bindare nello shader ..)

	// #2 Stage - INPUT ASSEMBLY
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType				   = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology			   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// Tipo della primitiva con cui assemblare i vertici
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE; // Abilità di ripartire con una nuova strip

	// #3 Stage - VIEWPORT & SCISSOR
	// InfoStruct per la viewport
	VkViewport viewport = {};
	viewport.x			= 0.f;											// Valore iniziale delle coordinate
	viewport.y			= 0.f;											// Valore iniziale delle coordinate
	viewport.width		= static_cast<float>(m_swapChainExtent.width);	// Larghezza della viewport
	viewport.height		= static_cast<float>(m_swapChainExtent.height);	// Altezza della viewport
	viewport.minDepth	= 0.0f;											// Minima profondità del framebuffer
	viewport.maxDepth	= 1.0f;											// Massima profondità del framebuffer
	
	//InfoStruct per la scissor
	VkRect2D scissor = {};				  
	scissor.offset	 = { 0,0 };			  // Offset da cui iniziare a tagliare la regione
	scissor.extent	 = m_swapChainExtent; // Extent che descrive la regione da tagliare

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType		  = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports	  = &viewport;
	viewportStateCreateInfo.scissorCount  = 1;
	viewportStateCreateInfo.pScissors	  = &scissor;

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
	rasterizerCreateInfo.sType					 = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;	// Tipo di struttura
	rasterizerCreateInfo.depthClampEnable		 = VK_FALSE;													// Disabilita il clamping della profondità (schiaccia gli oggetti -> problema con ombre)
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;													// Scarta i fragments, non disegna (viene utilizzato per recuperare i dati soltanto dal geometry/tess)
	rasterizerCreateInfo.polygonMode			 = VK_POLYGON_MODE_FILL;										// Come si vuole colorare la primitiva
	rasterizerCreateInfo.lineWidth				 = 1.0f;														// Quanto spessa le linee vengono disegnate.
	rasterizerCreateInfo.cullMode				 = VK_CULL_MODE_BACK_BIT;										// Quale faccia del triangolo non disegnare
	rasterizerCreateInfo.frontFace				 = VK_FRONT_FACE_CLOCKWISE;										// Rispetto da dove stiamo guardando il triangolo, se i punti sono stati disegnati in ordine orario allora stiamo guardano la primitiva da di fronte
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;															// Evita la problematica della Shadow Acne (aggiunge depth bias ai fragments)
	
	// #6 Stage - MULTISAMPLING
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.sType				   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;	// Tipo di struttura dati
	multisampleCreateInfo.sampleShadingEnable  = VK_FALSE;													// Disabilita 
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;										// Numero di sample utilizzati per fragment

	// #7 Stage - Blending
	// Decide come mescolare un nuovo colore in un fragment, utilizzando il vecchio valore
	// Equazione di blending [LERP] = (srcColour*newColour)colorBlendOp(dstColourBlendFactor * oldColour)
	VkPipelineColorBlendAttachmentState colourStateAttachment = {};	
		colourStateAttachment.colorWriteMask  = VK_COLOR_COMPONENT_A_BIT |			// Colori su cui applicare il blending
												VK_COLOR_COMPONENT_R_BIT |
												VK_COLOR_COMPONENT_B_BIT |
												VK_COLOR_COMPONENT_G_BIT;
	colourStateAttachment.blendEnable			= VK_TRUE;							// Abilita il blending
	// blending [LERP] = (VK_BLEND_FACTOR_SRC_ALPHA*newColour)colorBlendOp(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColour)
	//			[LERP] = (newColourAlpha * newColour) + ((1-newColourAlpha) * oldColour)
	colourStateAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			 // srcColour -> Valore Alpha del colore
	colourStateAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Inversione di alpha
	colourStateAttachment.colorBlendOp		  = VK_BLEND_OP_ADD;					 // L'operazione di blending è una somma

	// (1*newAlpha) + (0*oldAlpha) = newAlpha
	colourStateAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;					// Moltiplica il nuovo alpha value per 1
	colourStateAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;					// Moltiplica il vecchio alpha value per 0
	colourStateAttachment.alphaBlendOp		  = VK_BLEND_OP_ADD;						// Potremmo utilizzare anche una sottrazione ed il risultato sarebbe il medesimo
	
	VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
	colourBlendingCreateInfo.sType			 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; // Tipo struttura dati
	colourBlendingCreateInfo.logicOpEnable	 = VK_FALSE;												 // Un alternativa ai calcoli è quella di usare operazioni logiche
	colourBlendingCreateInfo.attachmentCount = 1;														 // Quanti attachment stiamo utilizzando
	colourBlendingCreateInfo.pAttachments	 = &colourStateAttachment;									 // Puntatore a gli allegati

	// -- PIPELINE LAYOUT --
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; // Tipo di struttura dati
	pipelineLayoutCreateInfo.setLayoutCount			= 0;											 // Il numero di set Layout
	pipelineLayoutCreateInfo.pSetLayouts			= nullptr;										 // Puntatore ai set layout
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;											 // Numero di range count
	pipelineLayoutCreateInfo.pPushConstantRanges	= nullptr;										 // Puntatore ai range count

	// Creazione del Pipeline Layout
	VkResult res = vkCreatePipelineLayout(m_mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout.");
	}

	// TODO: Impostare la profondità del deth stencing test

	
	// Creazione della pipeline
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType				= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; // Tipo di struttura dati
	pipelineCreateInfo.stageCount			= 2;											   // Stage count 
	pipelineCreateInfo.pStages				= shaderStages;									   // Lista degli shaders
	pipelineCreateInfo.pVertexInputState	= &vertexInputCreateInfo;						   // Vertex Stage
	pipelineCreateInfo.pInputAssemblyState	= &inputAssemblyCreateInfo;						   // Input Assembly Stage
	pipelineCreateInfo.pViewportState		= &viewportStateCreateInfo;						   // ViewPort Stage
	pipelineCreateInfo.pDynamicState		= nullptr;										   // Dynamic States Stage
	pipelineCreateInfo.pRasterizationState	= &rasterizerCreateInfo;						   // Rasterization Stage
	pipelineCreateInfo.pMultisampleState	= &multisampleCreateInfo;						   // Multisampling Stage
	pipelineCreateInfo.pColorBlendState		= &colourBlendingCreateInfo;					   // Colout blending Stage
	pipelineCreateInfo.pDepthStencilState	= nullptr;										   // Stencil Depth Stage
	pipelineCreateInfo.layout				= m_pipelineLayout;								   // Layout Stage
	pipelineCreateInfo.renderPass			= m_renderPass;									   // RenderPass Stage
	pipelineCreateInfo.subpass				= 0;											   // Subpass utilizzati nel RenderPass
	
	// Le derivate delle pipeline permettono di derivare da un altra pipeline (soluzione ottimale)
	pipelineCreateInfo.basePipelineHandle   = VK_NULL_HANDLE; // Pipeline da cui derivare
	pipelineCreateInfo.basePipelineIndex	= -1;			  // Indice della pipeline da cui derivare (in caso in cui siano passate molteplici)

	// Creazione della pipeline grafica
	res = vkCreateGraphicsPipelines(m_mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}

	// -- Distruzione dei moduli shader --
	vkDestroyShaderModule(m_mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

// Costruisce il RenderPass (e i subpass)
void VulkanRenderer::createRenderPass()
{
	// Attachment
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.format			= m_swapChainImageFormat;			// Formato dell'immagini utilizzato
	colourAttachment.samples		= VK_SAMPLE_COUNT_1_BIT;			// Numero di samples da utilizzare per il multisampling
	colourAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;		// Dopo aver caricato il colourAttachment performerà un operazione (pulisce l'immagine)
	colourAttachment.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;		// Dopo che abbiamo terminato con il renderpass si esegue un operazione di store ()
	colourAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // Descrive cosa fare con lo stencil prima del rendering
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Descrive cosa fare con lo stencil dopo il rendering

	// FrameBuffer data vengono salvate come immagini, ma alle immagini è possibile utilizzare differenti layout dei dati
	// questo per motivi di ottimizzazione nell'uso di alcune operazioni
	// È presente un layout intermedio che verrà svolto dai subpasses
	colourAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;		// Layout dell'immagini prima del render pass
	colourAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Layout delle immagini dopo il render pass (nel quale viene cambiato)

	// SubPass
	// Per passare gli attachment si utilizzano degli attachment reference che sono degli indici presenti
	// all'interno di una lista (colourAttachment) che viene passata alla createInfo del RenderPass
	VkAttachmentReference colourAttachmentReference = {};
	colourAttachmentReference.attachment			= 0;										// L'indice dell'elemento nella lista degli attachments definita nel renderPass ("colourAttachment")
	colourAttachmentReference.layout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Formato ottimale per il colour attachment (conversione intermedia)

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint	 = VK_PIPELINE_BIND_POINT_GRAPHICS; // Viene bindato alla pipeline grafica
	subpass.colorAttachmentCount = 1;								// Numero di ColourAttachment
	subpass.pColorAttachments	 = &colourAttachmentReference;		// Passo i riferimenti (uno solo) a gli elementi della lista
	
	// SubPass Dependencies
	// Abbiamo il bisogno di determinare quando la transizione del layout 
	// avviene utilizzando le subpass dependencies (transizioni implicite dei layout)
	std::array<VkSubpassDependency, 2> subpassDependencies;
		
	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL

	// Prima conversione da VK_IMAGE_LAYOUT_UNDEFINED a VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	subpassDependencies[0].srcSubpass	 = VK_SUBPASS_EXTERNAL;							  // La dipendenza ha un ingresso esterno (indice)
	subpassDependencies[0].srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		  // La conversione deve avvenire dopo termine dell'ingresso esterno
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;					  // Prima di effettuare la conversione deve essere letto

	subpassDependencies[0].dstSubpass	   = 0;												// La dipendenza ha come destinazione il primo SubPass della lista 'subpass'
	subpassDependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // La conversione deve avvenire prima del Color Output
	subpassDependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |			// La conversione deve avvenire prima che si provi a leggere o scrivere
										     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;												// Disabita eventuali flags inerenti all'esecuzione e memoria

	// Seconda conversione da VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL a VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	subpassDependencies[1].srcSubpass	 = 0;											  // Indice del primo subPass
	subpassDependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // La conversione deve avvenire dopo l'output del final color
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |		  // La conversione deve avvenire dopo lettura/scrittura
										   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;		 

	subpassDependencies[1].dstSubpass	   = VK_SUBPASS_EXTERNAL;				   // La dipendenza ha come destinazione l'esterno
	subpassDependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // La conversione deve avvenire prima della fine della pipeline
	subpassDependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;			   // La conversione deve avvenire prima della lettura
	subpassDependencies[1].dependencyFlags = 0;									   // Disabita eventuali flags inerenti all'esecuzione e memoria


	// Create info per il Render Pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType					= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;		 // Tipo di struttura dati
	renderPassCreateInfo.attachmentCount		= 1;												 // Numero di attachment
	renderPassCreateInfo.pAttachments			= &colourAttachment;								 // Puntatore a gli attachments
	renderPassCreateInfo.subpassCount			= 1;												 // Numero di subpasses
	renderPassCreateInfo.pSubpasses				= &subpass;											 // Puntatore ai subpasses
	renderPassCreateInfo.dependencyCount		= static_cast<uint32_t>(subpassDependencies.size()); // Numero di dependencies
	renderPassCreateInfo.pDependencies			= subpassDependencies.data();						 // Puntatore alle dependencies

	// Creazione del RenderPass
	VkResult res = vkCreateRenderPass(m_mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass);
	
	// Controllo del renderPass
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Render Pass!");
	}
}

// Creazione dei framebuffers e caricamento nel vettore 'm_swapChainFrameBuffers'
void VulkanRenderer::createFramebuffers()
{
	// Dimensione pari al numero di immagini contenute nella swapchain
	m_swapChainFrameBuffers.resize(m_swapChainImages.size());

	// Creiamo un framebuffer per ogni immagine della swapchain
	for (size_t i = 0; i < m_swapChainFrameBuffers.size(); ++i)
	{
		std::array<VkImageView, 1> attachments = {
			m_swapChainImages[i].imageView
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType			  = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; // Tipo di struttura dati
		frameBufferCreateInfo.renderPass	  = m_renderPass;							   // Render Pass
		frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // Numero degli attachments
		frameBufferCreateInfo.pAttachments	  = attachments.data();						   // Lista degli attachments 1:1 con il Render Pass
		frameBufferCreateInfo.width			  = m_swapChainExtent.width;				   // Framebuffer width
		frameBufferCreateInfo.height		  = m_swapChainExtent.height;				   // Framebuffer height
		frameBufferCreateInfo.layers		  = 1;										   // Framebuffer layers

		// Creazione del framebuffer
		VkResult result = vkCreateFramebuffer(m_mainDevice.logicalDevice, &frameBufferCreateInfo, nullptr, &m_swapChainFrameBuffers[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Framebuffer!");
		}

	}
}

// Creazione di una command pool
void VulkanRenderer::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType			  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; // 
	poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily;		//
	
	// Creazione di una command pool per una graphics queue
	VkResult res = vkCreateCommandPool(m_mainDevice.logicalDevice, &poolInfo, nullptr, &m_graphicsComandPool);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Command Pool!");
	}
}

void VulkanRenderer::createCommandBuffers()
{
	// Command buffer deve potere contenere tutti i frame buffers
	m_commandBuffer.resize(m_swapChainFrameBuffers.size());

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType			   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; //
	cbAllocInfo.commandPool		   = m_graphicsComandPool;							  //
	cbAllocInfo.level			   = VK_COMMAND_BUFFER_LEVEL_PRIMARY;				  // VK_COMMAND_BUFFER_LEVEL_PRIMARY : Buffer che viene inviato direttamente sulla queue. 
																					  // VK_COMMAND_BUFFER_LEVEL_SECONDARY : Il buffer non può essere chiamato direttamente, ma  da altri buffers via "vkCmdExecureCommands".
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffer.size()); // Quantità di commandbuffers che verranno creati (non si usa un for)

	// Salva tutti i command buffers dentro 'm_commandBuffer'
	VkResult res = vkAllocateCommandBuffers(m_mainDevice.logicalDevice, &cbAllocInfo, m_commandBuffer.data());
	
	if (res != VK_SUCCESS)
	{
		std::runtime_error("Failed to allocate Command Buffers!");
	}
}

void VulkanRenderer::createSynchronisation()
{
	m_imageAvailable.resize(MAX_FRAME_DRAWS);
	m_renderFinished.resize(MAX_FRAME_DRAWS);
	m_drawFences.resize(MAX_FRAME_DRAWS);

	// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Creazione della fence
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


	for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
	{
		if (vkCreateSemaphore(m_mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS || 
			vkCreateSemaphore(m_mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(m_mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &m_drawFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphores and/or Fence!");
		}
	}
}

// Resituisce lo shader module dato uno specifico SPIR-V
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
	// Creazione delle informazioni necessarie per la costruzione del modulo
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;	  // Tipo di struttura dati
	shaderModuleCreateInfo.codeSize = code.size();									  // Dimensione del codice
	shaderModuleCreateInfo.pCode	= reinterpret_cast<const uint32_t*>(code.data()); // Reinterpretazione del puntatore a const char

	// Costruzione dello shader module
	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(m_mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

	// Controllo che lo shader modula sia stato costruito con successo
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a shader module!");
	}

	return shaderModule;
}

// Distruttore
VulkanRenderer::~VulkanRenderer()
{
	cleanup();
}

// Pulisce il Vulkan Renderer
void VulkanRenderer::cleanup()
{
	// Aspetta finchè nessun azione sia eseguita sul device semza distruggere niente
	vkDeviceWaitIdle(m_mainDevice.logicalDevice);

	firstMesh.destroyVertexBuffer();

	// Distrugge i semafori
	for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
	{
		vkDestroySemaphore(m_mainDevice.logicalDevice, m_renderFinished[i], nullptr);
		vkDestroySemaphore(m_mainDevice.logicalDevice, m_imageAvailable[i], nullptr);
		vkDestroyFence(m_mainDevice.logicalDevice, m_drawFences[i], nullptr);
	}

	// Distrugge la commandpool
	vkDestroyCommandPool(m_mainDevice.logicalDevice, m_graphicsComandPool, nullptr);

	// Distrugge tutti i framebuffers
	for (auto framebuffer : m_swapChainFrameBuffers)
	{
		vkDestroyFramebuffer(m_mainDevice.logicalDevice, framebuffer, nullptr);
	}

	// Distrugge la pipeline grafica
	vkDestroyPipeline(m_mainDevice.logicalDevice, m_graphicsPipeline, nullptr);

	// Distrugge la pipeline layout
	vkDestroyPipelineLayout(m_mainDevice.logicalDevice, m_pipelineLayout, nullptr);

	// Distrugge il render pass
	vkDestroyRenderPass(m_mainDevice.logicalDevice, m_renderPass, nullptr);

	// Distrugge le immagini nella SwapChain
	for (auto image : m_swapChainImages)
	{
		vkDestroyImageView(m_mainDevice.logicalDevice, image.imageView, nullptr);
	}

	// Distrugge la swapchain
	vkDestroySwapchainKHR(m_mainDevice.logicalDevice, m_swapchain, nullptr);

	// Distrugge la surface
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);	// Distrugge la Surface (GLFW si utilizza solo per settarla)

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);	// Distruggo il dispositivo logico
	vkDestroyInstance(m_instance, nullptr);					// Distruggo l'istanza di Vulkan.
}
