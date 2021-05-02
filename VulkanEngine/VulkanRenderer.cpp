#include "pch.h"

#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
	m_VulkanInstance				= 0;
	m_Surface						= 0;

	
	m_descriptorPool				= 0;
	m_descriptorSetLayout			= 0;
	m_pushCostantRange				= {};

	m_depthBufferImage				= 0;
	m_depthBufferImageView			= 0;
	m_depthBufferImageMemory		= 0;
	m_depthFormat					= VK_FORMAT_UNDEFINED;

	m_graphicsComandPool			= 0;

	m_graphicsQueue					= 0;
	m_presentationQueue				= 0;

	m_MainDevice.LogicalDevice		= 0;
	m_MainDevice.PhysicalDevice		= 0;

	m_UboViewProjection.projection	= glm::mat4(1.f);
	m_UboViewProjection.view		= glm::mat4(1.f);

	m_modelTransferSpace			= 0;
	m_modelUniformAlignment			= 0;
	m_minUniformBufferOffset		= 0;
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

		m_SwapChainHandler = SwapChainHandler(m_MainDevice.PhysicalDevice, m_MainDevice.LogicalDevice, m_Surface, m_Window);
		m_SwapChainHandler.CreateSwapChain(m_queueFamilyIndices);

		createDescriptorSetLayout();
		m_RenderPassHandler = RenderPassHandler(m_MainDevice, m_SwapChainHandler);
		createPushCostantRange();

		m_GraphicPipeline = GraphicPipeline(m_MainDevice, m_SwapChainHandler, m_RenderPassHandler, m_descriptorSetLayout, m_TextureObjects, m_pushCostantRange);

		createDepthBufferImage();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createTextureSampler();
		//allocateDynamicBufferTransferSpace(); // DYNAMIC UBO
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createSynchronisation();

		// Creazione delle matrici che verranno caricate sui Descriptor Sets
		m_UboViewProjection.projection = glm::perspective(glm::radians(45.0f),
			static_cast<float>(m_SwapChainHandler.GetExtentWidth()) / static_cast<float>(m_SwapChainHandler.GetExtentHeight()),
			0.1f, 100.f);

		m_UboViewProjection.view = glm::lookAt(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.0f, 0.f));

		// GLM threat Y-AXIS as Up-Axis, but in Vulkan the Y is down.
		m_UboViewProjection.projection[1][1] *= -1;

		// Creazione della mesh
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

		// Index Data
		std::vector<uint32_t> meshIndices = {
			0, 1, 2,
			2, 3, 0
		};

		int giraffeTexture = Utility::CreateTexture(m_MainDevice, m_TextureObjects, m_graphicsQueue, m_graphicsComandPool,"giraffe.jpg");

		m_meshList.push_back(Mesh(
			m_MainDevice,
			m_graphicsQueue, m_graphicsComandPool,
			&meshVertices, &meshIndices, giraffeTexture));

		m_meshList.push_back(Mesh(
			m_MainDevice,
			m_graphicsQueue, m_graphicsComandPool,
			&meshVertices2, &meshIndices, giraffeTexture));

		glm::mat4 meshModelMatrix = m_meshList[0].getModel().model;
		meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(45.f), glm::vec3(.0f, .0f, 1.0f));
		m_meshList[0].setModel(meshModelMatrix);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}

void VulkanRenderer::updateModel(int modelID, glm::mat4 newModel)
{
	if (modelID >= m_meshList.size())
		return;

	m_meshList[modelID].setModel(newModel);
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
	vkWaitForFences(m_MainDevice.LogicalDevice, 1, &m_SyncObjects[m_CurrentFrame].InFlight, VK_TRUE, UINT64_MAX);

	// Metto la Fence ad UNSIGNALED (la GPU deve aspettare per la prossima operazione di draw)
	vkResetFences(m_MainDevice.LogicalDevice, 1, &m_SyncObjects[m_CurrentFrame].InFlight);
	
	// Recupero l'index della prossima immagine disponibile della SwapChain,
	// e mette SIGNALED il relativo semaforo 'm_imageAvailable' per avvisare
	// che l'immagine è pronta ad essere utilizzata.
	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_MainDevice.LogicalDevice, m_SwapChainHandler.GetSwapChain(),
						  std::numeric_limits<uint64_t>::max(),
					      m_SyncObjects[m_CurrentFrame].ImageAvailable, VK_NULL_HANDLE, &imageIndex);


	recordCommands(imageIndex);

	// Copia nell'UniformBuffer della GPU le matrici m_uboViewProjection
	updateUniformBuffers(imageIndex);

	// Stages dove aspettare che il semaforo sia SIGNALED (all'output del final color)
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	
	};
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	submitInfo.waitSemaphoreCount	= 1;											   // Numero dei semafori da aspettare
	submitInfo.pWaitSemaphores		= &m_SyncObjects[m_CurrentFrame].ImageAvailable; // Semaforo di disponibilità dell'immagine (aspetta che sia SIGNALED)
	submitInfo.pWaitDstStageMask	= waitStages;									   // Stage dove iniziare la semaphore wait (output del final color, termine della pipeline)
	
	submitInfo.commandBufferCount	= 1;								 // Numero di Command Buffer da inviare alla Queue
	submitInfo.pCommandBuffers		= &m_commandBuffer[imageIndex];		 // Il Command Buffer da inviare non è quello corrente, ma è quello dell'immagine disponile
	
	submitInfo.signalSemaphoreCount = 1;												// Numero di semafori a cui segnalare na volta che il CommandBuffer ha terminato
	submitInfo.pSignalSemaphores	= &m_SyncObjects[m_CurrentFrame].RenderFinished; // Semaforo di fine Rendering (verrà messo a SIGNALED)


	// L'operazione di submit del CommandBuffer alla Queue accade se prima del termine della Pipeline
	// è presente una nuova immagine disponibile, cosa garantita trammite una semaphore wait su 'm_imageAvailable' 
	// posizionata proprio prima dell'output stage della Pipeline.
	// Una volta che l'immagine sarà disponibile verranno eseguite le operazioni del CommandBuffer 
	// sulla nuova immagine disponibile. Al termine delle operazioni del CommandBuffer il nuovo render sarà pronto
	// e verrà avvisato con il semaforo 'm_renderFinished'.
	// Inoltre al termine del render verrà anche avvisato il Fence, per dire che è possibile effettuare
	// l'operazione di drawing a schermo
	VkResult result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_SyncObjects[m_CurrentFrame].InFlight);
	
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

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present the image!");
	}

	// Passa al prossimo frame
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::CreateInstance()
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
	appInfo.pEngineName			= "VULKAN RENDERER";				  // Nome dell'engine
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
	
	VkResult res = vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance);					   // Crea un istanza di Vulkan.

	if (res != VK_SUCCESS) // Nel caso in cui l'istanza non venga creata correttamente, alza un eccezione runtime.
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}

	if (enableValidationLayers)
	{
		//DebugMessanger::SetupDebugMessenger(m_instance);
		DebugMessanger::GetInstance()->SetupDebugMessenger(m_VulkanInstance);
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

void VulkanRenderer::recordCommands(uint32_t currentImage)
{
	// Informazioni sul come inizializzare ogni Command Buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Il buffer può essere re-inviato al momento della resubmit

	// Informazioni sul come inizializzare il RenderPass (necessario solo per le applicazioni grafiche)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType			  = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; 
	renderPassBeginInfo.renderPass		  = m_RenderPassHandler.GetRenderPass();	   // RenderPass
	renderPassBeginInfo.renderArea.offset = { 0, 0 };		   // Start point del RenderPass (in Pixels)
	renderPassBeginInfo.renderArea.extent = m_SwapChainHandler.GetExtent(); // Dimensione delal regione dove eseguire il RenderPass (partendo dall'offset)

	// Valori che vengono utilizzati per pulire l'immagine (background color)
	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = { 0.6f, 0.65f, 0.4f, 1.0f };
	clearValues[1].depthStencil.depth = 1.0f;

	renderPassBeginInfo.pClearValues	= clearValues.data(); // Puntatore ad un array di ClearValues
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());		   // Numero di ClearValues

	// Associo un Framebuffer per volta
	renderPassBeginInfo.framebuffer = m_SwapChainHandler.GetFrameBuffer(currentImage); 

	// Inizia a registrare i comandi nel Command Buffer
	VkResult res = vkBeginCommandBuffer(m_commandBuffer[currentImage], &bufferBeginInfo);
		
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to start recording a Command Buffer!");
	}

	// Inizio del RenderPass
	// VK_SUBPASS_CONTENTS_INLINE indica che tutti i comandi verranno registrati direttamente sul CommandBuffer primario
	// e che il CommandBuffer secondario non debba essere eseguito all'interno del Subpass
	vkCmdBeginRenderPass(m_commandBuffer[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Binding della Pipeline Grafica ad un Command Buffer.
		// VK_PIPELINE_BIND_POINT_GRAPHICS, indica il tipo Binding Point della Pipeline (in questo grafico per la grafica).
		// (È possibile impostare molteplici pipeline e switchare, per esempio una versione DEFERRED o WIREFRAME)
		vkCmdBindPipeline(m_commandBuffer[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline.GetPipeline());
				
		for (size_t j = 0; j < m_meshList.size(); ++j)
		{
			VkBuffer vertexBuffers[] = { m_meshList[j].getVertexBuffer() }; // Buffer da bindare prima di essere disegnati
			VkDeviceSize offsets[]   = { 0 };
					
			// Binding del Vertex Buffer di una Mesh ad un Command Buffer
			vkCmdBindVertexBuffers(m_commandBuffer[currentImage], 0, 1, vertexBuffers, offsets);

			// Bind del Index Buffer di una Mesh ad un Command Buffer, con offset 0 ed usando uint_32
			vkCmdBindIndexBuffer(m_commandBuffer[currentImage], m_meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			Model m = m_meshList[j].getModel();

			vkCmdPushConstants(m_commandBuffer[currentImage], m_GraphicPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Model), &m);

			std::array<VkDescriptorSet, 2> descriptorSetGroup = { m_descriptorSets[currentImage], m_TextureObjects.SamplerDescriptorSets[m_meshList[j].getTexID()] };

			// Binding del Descriptor Set ad un Command Buffer DYNAMIC UBO
			//uint32_t dynamicOffset = static_cast<uint32_t>(m_modelUniformAlignment) * static_cast<uint32_t>(j);

			vkCmdBindDescriptorSets(m_commandBuffer[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_GraphicPipeline.GetLayout(), 0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);

			// Invia un Comando di IndexedDraw ad un Command Buffer
			vkCmdDrawIndexed(m_commandBuffer[currentImage], m_meshList[j].getIndexCount(), 1, 0, 0, 0);
		}

	// Termine del Render Pass
	vkCmdEndRenderPass(m_commandBuffer[currentImage]);

	// Termian la registrazione dei comandi
	res = vkEndCommandBuffer(m_commandBuffer[currentImage]);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to stop recording a Command Buffer!");
	}
}

// DYNAMIC UBO (NON UTILIZZATI)
void VulkanRenderer::allocateDynamicBufferTransferSpace()
{
	// Allineamento dei dati per la Model
	/*
		Si vuole scoprire l'allineamento da utilizzare per la Model Uniform. Si utilizzano varie operazioni di bitwise.
		e.g, fingiamo che la dimensione minima di offset sia di 32-bit quindi rappresentata da 00100000.
		Al di sopra di questa dimensione di bit sono accettati tutti i suoi multipli (32,64,128,...).

		Quindi singifica che se la dimensione della Model è di 16-bit non sarà presente un allineamento per la memoria
		che sia valido, mentre 128-bit andrà bene. Per trovare questo allineamento si utilizza una maschera di per i 32-bit 
		in modo da poterla utilizzare con l'operazione BITWISE di AND (&) con la dimensione effettiva in byte della Model.
		
		Il risultato di questa operazioni saranno i byte dell'allineamento necessari.
		Per la creazione della maschera stessa si utilizza il complemento a 1 seguito dall'inversione not (complemento a 1).

		32-bit iniziali (da cui voglio creare la maschera) : 00100000
		32-bit meno 1 (complemento a 2) : 00011111
		32-bit complemento a 1 : 11100000 (maschera di bit risultante)

		maschera : ~(minOffset -1)
		Allineamento risultante : sizeof(UboModel) & maschera

		Tutto funziona corretto finchè non si vuole utilizzare una dimensione che non è un multiplo diretto della maschera
		per esempio 00100010, in questo caso la mascherà risultante sarà 11100000 ma questa da un allineamento di 00100000.
		cosa che non è corretta perchè si perde il valore contenuto nella penultima cifra!
		
		Come soluzione a questa problematica si vuole spostare la cifra che verrebbe persa subito dopo quella risultante
		dall'allineamento, in questo modo l'allineamento dovrebbe essere corretto.

		Per fare ciò aggiungiamo 32 bit e facciamo il complemento a 2 su questi 32-bit.
		00100000 => 00011111
		Ora aggiungendo questi bit alla dimensione della Model dovrebbe essere protetta.
	*/

	/*unsigned long long const offsetMask			= ~(m_minUniformBufferOffset - 1);
	unsigned long long const protectedModelSize = (sizeof(Model) + m_minUniformBufferOffset - 1);
	m_modelUniformAlignment = protectedModelSize & offsetMask;

	// Allocazione spazio in memoria per gestire il dynamic buffer che è allineato, e con un limite di oggetti
	m_modelTransferSpace = (Model*)_aligned_malloc(m_modelUniformAlignment * MAX_OBJECTS, m_modelUniformAlignment);*/
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

	//m_minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;// serve per DYNAMIC UBO

}

// Controlla se un device è adatto per l'applicazione
bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
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
	getQueueFamiliesIndices(device);

	// Controlla che le estensioni richieste siano disponibili nel dispositivo fisico
	bool const extensionSupported = checkDeviceExtensionSupport(device);


	bool swapChainValid	= false;

	// Se le estensioni richieste sono supportate (quindi Surface compresa), si procede con la SwapChain
	if (extensionSupported)
	{						
		SwapChainDetails swapChainDetails = m_SwapChainHandler.GetSwapChainDetails(device, m_Surface);
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}

	return m_queueFamilyIndices.isValid() && extensionSupported && swapChainValid /*&& deviceFeatures.samplerAnisotropy*/;	// Il dispositivo è considerato adatto se :
																					// 1. Gli indici delle sue Queue Families sono validi
																					// 2. Se le estensioni richieste sono supportate
																					// 3. Se la Swap Chain è valida 
																					// 4. Supporta il sampler per l'anisotropy (basta controllare una volta che lo supporti la mia scheda video poi contrassegno l'esistenza nel createLogicalDevice)
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

// Crea il dispositivo logico
void VulkanRenderer::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { m_queueFamilyIndices.GraphicsFamily , m_queueFamilyIndices.PresentationFamily };

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
		m_queueFamilyIndices.GraphicsFamily, 
		0,
		&m_graphicsQueue);
		
	vkGetDeviceQueue(							 // Salvo il riferimento alla Presentation Queue del device logico
		m_MainDevice.LogicalDevice,				 // nella variabile 'm_presentationQueue'. Siccome è la medesima cosa della
		m_queueFamilyIndices.PresentationFamily, // queue grafica, nel caso in cui sia presente una sola Queue nel device 
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
				m_queueFamilyIndices.GraphicsFamily = queueIndex;
			}

			VkBool32 presentationSupport = false;

			/*	Query per chiedere se è supportata la Presentation Mode nella Queue Family.
				Solitamente le Queue Family appartenenti alla grafica la supportano.
				Questa caratteristica è obbligatoria per presentare l'immagine dalla SwapChain alla Surface.*/
			vkGetPhysicalDeviceSurfaceSupportKHR(device, queueIndex, m_Surface, &presentationSupport);

			// Se la Queue Family supporta la Presentation Mode, salvo l'indice nel Renderer (solitamente sono gli stessi indici)
			if (presentationSupport)
			{
				m_queueFamilyIndices.PresentationFamily = queueIndex;
			}

			if (m_queueFamilyIndices.isValid())
			{
				break;
			}

			++queueIndex;
		}
	}
}

void VulkanRenderer::createDepthBufferImage()
{
	// Depth value of 32bit expressed in floats + stencil buffer (non lo usiamo ma è utile averlo disponibile)
	// Nel caso in cui lo stencil buffer non sia disponibile andiamo a prendere solo il depth buffer 32 bit.
	// Se non è disponibile neanche quello da 32 bit si prova con quello di 24 (poi non proviamo più).
	m_depthFormat = Utility::ChooseSupportedFormat(m_MainDevice, { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	VkImage depthBufferImage = Utility::CreateImage(m_MainDevice, m_SwapChainHandler.GetExtentWidth(), m_SwapChainHandler.GetExtentHeight(),
		m_depthFormat, VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_depthBufferImageMemory);

	m_depthBufferImageView = Utility::CreateImageView(m_MainDevice.LogicalDevice, depthBufferImage, m_depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// Creazione dei Framebuffer, effettuano una connessione tra il RenderPass e le immagini. Utilizzando rispettivamente Attachments ed ImageView
void VulkanRenderer::createFramebuffers()
{

	m_SwapChainHandler.ResizeFrameBuffers();

	// Creiamo un framebuffer per ogni immagine della swapchain
	for (size_t i = 0; i <  m_SwapChainHandler.NumOfFrameBuffers(); ++i)
	{
		std::array<VkImageView, 2> attachments = {
			m_SwapChainHandler.GetSwapChainImageView(i),
			m_depthBufferImageView
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType			  = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass	  = m_RenderPassHandler.GetRenderPass();							   // RenderPass
		frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // Numero degli attachments
		frameBufferCreateInfo.pAttachments	  = attachments.data();						   // Lista degli attachments 1:1 con il RenderPass
		frameBufferCreateInfo.width			  = m_SwapChainHandler.GetExtentWidth();				   // Framebuffer width
		frameBufferCreateInfo.height		  = m_SwapChainHandler.GetExtentHeight();				   // Framebuffer height
		frameBufferCreateInfo.layers		  = 1;										   // Framebuffer layers

		VkResult result = vkCreateFramebuffer(m_MainDevice.LogicalDevice, &frameBufferCreateInfo, nullptr, &m_SwapChainHandler.GetFrameBuffer(i));

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
	poolInfo.flags			  = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Il nostro CommandBuffer viene resettato ogni qualvolta si accede alla 'vkBeginCommandBuffer()'
	poolInfo.queueFamilyIndex = m_queueFamilyIndices.GraphicsFamily;			 // I comandi dei CommandBuffers funzionano solo per le Graphic Queues
	
	VkResult res = vkCreateCommandPool(m_MainDevice.LogicalDevice, &poolInfo, nullptr, &m_graphicsComandPool);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Command Pool!");
	}
}

// Allocazione dei Command Buffers (gruppi di comandi da inviare alle queues)
void VulkanRenderer::createCommandBuffers()
{
	// Tanti CommandBuffers quanti FramBuffers
	m_commandBuffer.resize(m_SwapChainHandler.NumOfFrameBuffers());

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool	= m_graphicsComandPool;				// Command Pool dalla quale allocare il Command Buffer
	cbAllocInfo.level		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// Livello del Command Buffer
																// VK_COMMAND_BUFFER_LEVEL_PRIMARY   : Il CommandBuffer viene inviato direttamente sulla queue. 
																// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Il CommandBuffer non può essere chiamato direttamente, ma da altri buffers via "vkCmdExecuteCommands".
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffer.size()); // Numero di CommandBuffers da allocare

	VkResult res = vkAllocateCommandBuffers(m_MainDevice.LogicalDevice, &cbAllocInfo, m_commandBuffer.data());
	
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Command Buffers!");
	}
}

void VulkanRenderer::createTextureSampler()
{
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType					  = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter				  = VK_FILTER_LINEAR;		// linear interpolation between the texels
	samplerCreateInfo.minFilter				  = VK_FILTER_LINEAR;		// quando viene miniaturizzata come renderizzarla (lerp)
	samplerCreateInfo.addressModeU			  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV			  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW			  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.borderColor			  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;			// è normalizzata
	samplerCreateInfo.mipmapMode			  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias			  = 0.0f;
	samplerCreateInfo.minLod				  = 0.0f;
	samplerCreateInfo.maxLod				  = 0.0f;
	samplerCreateInfo.anisotropyEnable		  = VK_TRUE;
	samplerCreateInfo.maxAnisotropy			  = 16;



	VkResult result = vkCreateSampler(m_MainDevice.LogicalDevice, &samplerCreateInfo, nullptr, &m_TextureObjects.TextureSampler);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Texture Sampler!");
	}

}

// Sincronizzazione : 1 semaforo per le immagini disponibili, 1 semaforo per le immagini presentabili/renderizzate, 1 fence per l'operazione di draw
void VulkanRenderer::createSynchronisation()
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

void VulkanRenderer::createDescriptorSetLayout()
{
	// View-Projection Binding Info
	VkDescriptorSetLayoutBinding viewProjectionLayoutBinding = {};
	viewProjectionLayoutBinding.binding		       = 0;									// Punto di binding nello Shader
	viewProjectionLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Tipo di Descriptor Set (Uniform Buffer Object)
	viewProjectionLayoutBinding.descriptorCount    = 1;									// Numero di Descriptor Set da bindare
	viewProjectionLayoutBinding.stageFlags		   = VK_SHADER_STAGE_VERTEX_BIT;		// Shaders Stage nel quale viene effettuato il binding
	viewProjectionLayoutBinding.pImmutableSamplers = nullptr;							// rendere il sampler immutable specificando il layout, serve per le textures

	// Model Binding Info (DYNAMIC UBO)
	/*VkDescriptorSetLayoutBinding modelLayoutBinding = {};
	modelLayoutBinding.binding			   = 1;											// Punto di binding nello Shader
	modelLayoutBinding.descriptorType	   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // Tipo di Descriptor Set (Dynamic Uniform Buffer Object)
	modelLayoutBinding.descriptorCount	   = 1;											// Numero di Descriptor Set da bindare
	modelLayoutBinding.stageFlags		   = VK_SHADER_STAGE_VERTEX_BIT;				// Shaders Stage nel quale viene effettuato il binding
	modelLayoutBinding.pImmutableSamplers = nullptr;									// rendere il sampler immutable specificando il layout, serve per le textures
	*/
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { viewProjectionLayoutBinding };

	// Crea il layout per il Descriptor Set
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size()); 
	layoutCreateInfo.pBindings    = layoutBindings.data();						  
	
	// Creazione del layout del Descriptor Set Layout
	VkResult result = vkCreateDescriptorSetLayout(m_MainDevice.LogicalDevice, &layoutCreateInfo, nullptr, &m_descriptorSetLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Set Layout");
	}

	// Create texture sampler descriptor set layout
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding			= 0;
	samplerLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount	= 1;
	samplerLayoutBinding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
	textureLayoutCreateInfo.sType		 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutCreateInfo.bindingCount = 1;
	textureLayoutCreateInfo.pBindings	 = &samplerLayoutBinding;

	result = vkCreateDescriptorSetLayout(m_MainDevice.LogicalDevice, &textureLayoutCreateInfo, nullptr, &m_TextureObjects.SamplerSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Set Layout!");
	}
}

void VulkanRenderer::createPushCostantRange()
{
	m_pushCostantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // Stage dove finiranno le push costant
	m_pushCostantRange.offset	  = 0;
	m_pushCostantRange.size		  = sizeof(Model);
}

void VulkanRenderer::createUniformBuffers()
{
	VkDeviceSize viewProjectionBufferSize = sizeof(m_UboViewProjection);
	//VkDeviceSize modelBufferSize		  = m_modelUniformAlignment * MAX_OBJECTS;

	// Un UBO per ogni immagine della Swap Chain
	m_viewProjectionUBO.resize(m_SwapChainHandler.NumOfSwapChainImages());
	m_viewProjectionUniformBufferMemory.resize(m_SwapChainHandler.NumOfSwapChainImages());
	/*
	m_modelDynamicUBO.resize(m_swapChainImages.size());
	m_modelDynamicUniformBufferMemory.resize(m_swapChainImages.size());*/

	for (size_t i = 0; i < m_SwapChainHandler.NumOfSwapChainImages(); i++)
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

void VulkanRenderer::createDescriptorPool()
{
	//

	/* CREATE UNIFORM DESCRIPTOR POOL */

	// VIEW - PROJECTION
	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.type			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;			   // Che tipo di descriptor contiene (non è un descriptor set ma sono quelli presenti negli shaders)
	vpPoolSize.descriptorCount = static_cast<uint32_t> (m_viewProjectionUBO.size()); // Quanti descriptor

	// MODEL (DYNAMIC UBO)
	/*VkDescriptorPoolSize modelPoolSize = {};
	modelPoolSize.type			  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelPoolSize.descriptorCount = static_cast<uint32_t>(m_modelDynamicUBO.size()); */

	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

	// Data per la creazione della pool
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType		 = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets		 = static_cast<uint32_t>(m_SwapChainHandler.NumOfSwapChainImages()); // un descriptor set per ogni commandbuffer/swapimage
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());											  // Quante pool
	poolCreateInfo.pPoolSizes	 = descriptorPoolSizes.data();		// Array di pool size

	// Creazione della Descriptor Pool
	VkResult res = vkCreateDescriptorPool(m_MainDevice.LogicalDevice, &poolCreateInfo, nullptr, &m_descriptorPool);
	
	if(res!= VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}
	
	/* TEXTURE SAMPLER DESCRIPTOR POOL */
	
	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = MAX_OBJECTS;

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
	samplerPoolCreateInfo.poolSizeCount = 1;
	samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

	VkResult result = vkCreateDescriptorPool(m_MainDevice.LogicalDevice, &samplerPoolCreateInfo, nullptr, &m_TextureObjects.SamplerDescriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}

}

void VulkanRenderer::createDescriptorSets()
{
	// Un Descriptor Set per ogni Uniform Buffer
	m_descriptorSets.resize(m_SwapChainHandler.NumOfSwapChainImages());

	// Lista di tutti i possibili layour che useremo dal set (?) non capito TODO
	std::vector<VkDescriptorSetLayout> setLayouts(m_SwapChainHandler.NumOfSwapChainImages(), m_descriptorSetLayout);

	// Informazioni per l'allocazione del descriptor set
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool		= m_descriptorPool;								 // Pool per l'allocazione dei Descriptor Set
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChainHandler.NumOfSwapChainImages()); // Quanti Descriptor Set allocare
	setAllocInfo.pSetLayouts		= setLayouts.data();							 // Layout da utilizzare per allocare i set (1:1)

	// Allocazione dei descriptor sets (molteplici)
	VkResult res = vkAllocateDescriptorSets(m_MainDevice.LogicalDevice, &setAllocInfo, m_descriptorSets.data());

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Descriptor Sets!");
	}

	// Aggiornamento dei bindings dei descriptor sets
	for (size_t i = 0; i < m_SwapChainHandler.NumOfSwapChainImages(); ++i)
	{
		// VIEW-PROJECTION DESCRIPTOR
		
		
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = m_viewProjectionUBO[i];		// Buffer da cui prendere i dati
		vpBufferInfo.offset = 0;							// Offset da cui partire
		vpBufferInfo.range  = sizeof(m_UboViewProjection);	// Dimensione dei dati

		VkWriteDescriptorSet vpSetWrite = {}; // DescriptorSet per m_uboViewProjection
		vpSetWrite.sType		   = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet		   = m_descriptorSets[i];
		vpSetWrite.dstBinding	   = 0;									// Vogliamo aggiornare quello che binda su 0
		vpSetWrite.dstArrayElement = 0;									// Se avessimo un array questo
		vpSetWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Indice nell'array da aggiornare
		vpSetWrite.descriptorCount = 1;									// Numero di descriptor da aggiornare
		vpSetWrite.pBufferInfo	   = &vpBufferInfo;						// Informazioni a riguardo dei dati del buffer da bindare
/*
		// MODEL DESCRIPTOR (DYNAMIC UBO)
		// Model Buffer Binding Info
		VkDescriptorBufferInfo modelBufferInfo = {};
		modelBufferInfo.buffer = m_modelDynamicUBO[i];
		modelBufferInfo.offset = 0;
		modelBufferInfo.range  = m_modelUniformAlignment;

		VkWriteDescriptorSet modelSetWrite = {};
		modelSetWrite.sType			  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		modelSetWrite.dstSet		  = m_descriptorSets[i];
		modelSetWrite.dstBinding	  = 1;										   // Vogliamo aggiornare quello che binda su 0
		modelSetWrite.dstArrayElement = 0;										   // Se avessimo un array questo
		modelSetWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // Indice nell'array da aggiornare
		modelSetWrite.descriptorCount = 1;										   // Numero di descriptor da aggiornare
		modelSetWrite.pBufferInfo	  = &modelBufferInfo;						   // Informazioni a riguardo dei dati del buffer da bindare
		*/
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

		// Aggiornamento dei descriptor sets con i nuovi dati del buffer/binding
		vkUpdateDescriptorSets(m_MainDevice.LogicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

// Copia dei dati m_uboViewProjection della CPU nell'UniformBufferObject della GPU
void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex)
{
	/* Copia dei dati CPU-GPU VIEW-PROJECTION */
	// Mappa l'UniformBufferObject nello spazio di indirizzamento dell'applicazione, salvando nel puntatore 'data' l'intervallo mappato sulla memoria.
	void* data;
	vkMapMemory(m_MainDevice.LogicalDevice, 
		m_viewProjectionUniformBufferMemory[imageIndex], 0, 
		sizeof(UboViewProjection), 0, &data);
	memcpy(data, &m_UboViewProjection, sizeof(UboViewProjection));
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
	cleanup();
}

void VulkanRenderer::cleanup()
{
	// Aspetta finchè nessun azione sia eseguita sul device senza distruggere niente
	// Tutte le operazioni effettuate all'interno della draw() sono in asincrono.
	// Questo significa che all'uscita del loop della init(), le operazionio di drawing
	// e di presentazione potrebbero ancora essere in corso ed eliminare le risorse mentre esse sono in corso è una pessima idea
	// quindi è corretto aspettare che il dispositivo sia inattivo prima di eliminare gli oggetti.
	vkDeviceWaitIdle(m_MainDevice.LogicalDevice);

	vkDestroyDescriptorPool(m_MainDevice.LogicalDevice, m_TextureObjects.SamplerDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_MainDevice.LogicalDevice, m_TextureObjects.SamplerSetLayout, nullptr);

	vkDestroySampler(m_MainDevice.LogicalDevice, m_TextureObjects.TextureSampler, nullptr);

	for (size_t i = 0; i < m_TextureObjects.TextureImages.size(); i++)
	{
		vkDestroyImageView(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImageViews[i], nullptr);
		vkDestroyImage(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImages[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_TextureObjects.TextureImageMemory[i], nullptr);
	}

	// Distruzione depthImage
	vkDestroyImageView(m_MainDevice.LogicalDevice, m_depthBufferImageView, nullptr);
	vkDestroyImage(m_MainDevice.LogicalDevice, m_depthBufferImage, nullptr);
	vkFreeMemory(m_MainDevice.LogicalDevice, m_depthBufferImageMemory, nullptr);

	// Libero la memoria allineata per la Model (DYNAMIC UBO)
	//_aligned_free(m_modelTransferSpace);

	// Distrugge della Pool di DescriptorSet
	vkDestroyDescriptorPool(m_MainDevice.LogicalDevice, m_descriptorPool, nullptr);

	// Distrugge del layout del DescriptorSet
	vkDestroyDescriptorSetLayout(m_MainDevice.LogicalDevice, m_descriptorSetLayout, nullptr);

	// Distrugge i buffers per i DescriptorSet (Dynamic UBO)
	/*
	for (size_t i = 0; i < m_swapChainImages.size(); i++)
	{
		vkDestroyBuffer(m_mainDevice.LogicalDevice, m_viewProjectionUBO[i], nullptr);
		vkFreeMemory(m_mainDevice.LogicalDevice, m_viewProjectionUniformBufferMemory[i], nullptr);
		
		vkDestroyBuffer(m_mainDevice.LogicalDevice, m_modelDynamicUBO[i], nullptr);
		vkFreeMemory(m_mainDevice.LogicalDevice, m_modelDynamicUniformBufferMemory[i], nullptr);
	}*/

	// Distrugge le Meshes
	for (size_t i = 0; i < m_meshList.size(); i++)
	{
		m_meshList[i].destroyBuffers();
	}

	// Distrugge i Semafori
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SyncObjects[i].RenderFinished, nullptr);
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SyncObjects[i].ImageAvailable, nullptr);
		vkDestroyFence(m_MainDevice.LogicalDevice, m_SyncObjects[i].InFlight, nullptr);
	}

	// Distrugge la Command Pool
	vkDestroyCommandPool(m_MainDevice.LogicalDevice, m_graphicsComandPool, nullptr);

	// Distrugge tutti i Framebuffers
	m_SwapChainHandler.DestroyFrameBuffers(m_MainDevice.LogicalDevice);



	m_SwapChainHandler.DestroySwapChainImages(m_MainDevice.LogicalDevice);
	m_SwapChainHandler.DestroySwapChain(m_MainDevice.LogicalDevice);

	// Distrugge la Surface
	vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);	// Distrugge la Surface (GLFW si utilizza solo per settarla)

	if (enableValidationLayers)
	{
		DebugMessanger::GetInstance()->Clear();
	}

	// Distrugge il device logico
	vkDestroyDevice(m_MainDevice.LogicalDevice, nullptr);
	
	// Distrugge l'istanza di Vulkan
	vkDestroyInstance(m_VulkanInstance, nullptr);					
}