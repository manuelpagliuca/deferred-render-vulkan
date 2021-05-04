#include "pch.h"

#include "SwapChainHandler.h"

SwapChainHandler::SwapChainHandler()
{
	m_Swapchain				= 0;
	m_MainDevice = {};
	m_VulkanDevice			= 0;
	m_VulkanSurface			= 0;
	m_SwapChainExtent		= {};
	m_SwapChainImageFormat	= {};
	m_GLFWwindow			= nullptr;
}
	
SwapChainHandler::SwapChainHandler(MainDevice &mainDevice, VkDevice&device, VkSurfaceKHR &surface, GLFWwindow * glfwWindow)
{
	m_Swapchain				= 0;
	m_MainDevice = mainDevice;
	m_VulkanDevice			= device;
	m_VulkanSurface			= surface;
	m_SwapChainExtent		= {};
	m_SwapChainImageFormat	= {};
	m_GLFWwindow			= glfwWindow;
}

VkSwapchainKHR* SwapChainHandler::GetSwapChainData()
{
	return &m_Swapchain;
}

VkSwapchainKHR& SwapChainHandler::GetSwapChain()
{
	return m_Swapchain;
}

SwapChainImage* SwapChainHandler::GetImage(uint32_t index)
{
	return &m_SwapChainImages[index];
}

void SwapChainHandler::PushImage(SwapChainImage swapChainImage)
{
	m_SwapChainImages.push_back(swapChainImage);
}

VkImageView& SwapChainHandler::GetSwapChainImageView(uint32_t index)
{
	return m_SwapChainImages[index].imageView;
}

size_t SwapChainHandler::NumOfSwapChainImages() const
{
	return m_SwapChainImages.size();
}

VkFramebuffer& SwapChainHandler::GetFrameBuffer(uint32_t index)
{
	return m_SwapChainFrameBuffers[index];
}

std::vector<VkFramebuffer>& SwapChainHandler::GetFrameBuffers()
{
	return m_SwapChainFrameBuffers;
}

void SwapChainHandler::PushFrameBuffer(VkFramebuffer frameBuffer)
{
	m_SwapChainFrameBuffers.push_back(frameBuffer);
}

void SwapChainHandler::ResizeFrameBuffers()
{
	m_SwapChainFrameBuffers.resize(m_SwapChainImages.size());
}

size_t SwapChainHandler::NumOfFrameBuffers() const
{
	return m_SwapChainFrameBuffers.size();
}


void SwapChainHandler::DestroyFrameBuffers(VkDevice device)
{
	for (auto framebuffer : m_SwapChainFrameBuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
}

void SwapChainHandler::DestroySwapChainImages(VkDevice device)
{
	for (auto image : m_SwapChainImages)
	{
		vkDestroyImageView(device, image.imageView, nullptr);
	}
}

void SwapChainHandler::DestroySwapChain(VkDevice device)
{
	vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
}

uint32_t SwapChainHandler::GetExtentWidth() const
{
	return m_SwapChainExtent.width;
}

uint32_t SwapChainHandler::GetExtentHeight() const
{
	return m_SwapChainExtent.height;
}

VkExtent2D& SwapChainHandler::GetExtent()
{
	return m_SwapChainExtent;
}

VkFormat& SwapChainHandler::GetSwapChainImageFormat()
{
	return m_SwapChainImageFormat;
}


void SwapChainHandler::CreateSwapChain(QueueFamilyIndices &t_QueueFamilyIndices)
{
// Crea la SwapChain e la salva all'interno del Renderer come 'm_swapChain'.
// Crea le Image Views e le carica assieme alle Image all'interno del vettore 'm_swapChainImages'

	// Preleva le informazioni della SwapChain (Capabilities, Formats, Presentation modes) a partire dalla Surface
	SwapChainDetails swapChainDetails = GetSwapChainDetails(m_MainDevice.PhysicalDevice, m_VulkanSurface);

	// Dalle precedenti informazioni seleziono le impostazioni migliori per la SwapChain 
	VkExtent2D extent				  = ChooseSwapExtent(swapChainDetails.surfaceCapabilities);
	VkSurfaceFormatKHR surfaceFormat  = ChooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode	  = ChooseBestPresentationMode(swapChainDetails.presentationModes);

	// Il numero di immagini è 1 immagine in più della quantità minima di immagini per il triple buffering (un immagine trattenuta nella queue)
	uint32_t imageCount				  = swapChainDetails.surfaceCapabilities.minImageCount + 1;

	// Se il numero massimo di immagini è positivo e questo numero è minore delle immagini che verranno utilizzate
	// Allora significa che il massimo numero di immagini disponibile non basta per supportare il triple-buffering
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
		swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType				= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface				= m_VulkanSurface;												  // Surface alla quale viene bindata la SwapChain
	swapChainCreateInfo.imageFormat			= surfaceFormat.format;									  // Formato delle immagini della SwapChain
	swapChainCreateInfo.imageColorSpace		= surfaceFormat.colorSpace;								  // Spazio colore delle immagini della SwapChain
	swapChainCreateInfo.presentMode			= presentMode;											  // Presentation Mode della SwapChain
	swapChainCreateInfo.imageExtent			= extent;												  // Extent (dimensioni in pixels) delle immagini della SwapChain 
	swapChainCreateInfo.minImageCount		= imageCount;											  // Minimo numero di immagini utilizzate dalla SwapChain
	swapChainCreateInfo.imageArrayLayers	= 1;													  // Numero di viste per una Surface multiview/stereo
	swapChainCreateInfo.imageUsage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;					  // Specifica l'utilizzo dell'immagine (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : le Image sono utilizzate per creare ImageView adatte ai colori)
	swapChainCreateInfo.preTransform		= swapChainDetails.surfaceCapabilities.currentTransform;  // Trasformazione relativa all'orientamento del motore di presentazione (se non combacia con la currentTransform della Surface viene calcolata durante la presentazione)
	swapChainCreateInfo.compositeAlpha		= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;					  // Come vengono gestite le immagini trasparenti con Surface esterne (e.g., 2 finestre che si sovrappongono)
	swapChainCreateInfo.clipped				= VK_TRUE;												  // Abilitare il clipping tra la Surface e l'ambiente esterno

	// Se le famiglie Graphics e Presentation sono diverse, allora la SwapChain deve trattare le immagini
	// in maniera condivisa e concorrente tra le due queues (non è la modalità più efficente).
	if (t_QueueFamilyIndices.GraphicsFamily != t_QueueFamilyIndices.PresentationFamily)
	{
		uint32_t queueFamilyIndices[]{
			static_cast<uint32_t>(t_QueueFamilyIndices.GraphicsFamily),
			static_cast<uint32_t>(t_QueueFamilyIndices.PresentationFamily)
		};

		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;	// Modalità concorrente e condivisa
		swapChainCreateInfo.queueFamilyIndexCount = 2;							// Numero di queue utilizzate
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;			// Array delle contenente i riferimenti alle Queue
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Modalità esclusiva (1 sola queue)
		swapChainCreateInfo.queueFamilyIndexCount = 1;						   // Numero di queue utilizzate
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;				   // Array delle contenente i riferimenti alle Queue
	}

	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // La SwapChain viene sostituita dall'attuale, è possibile fornire un link per trasferirne la responsabilità

	// Creazione della SwapChain e memorizzazione nel Renderer dentro 'm_swapchain'
	VkResult res = vkCreateSwapchainKHR(m_VulkanDevice, &swapChainCreateInfo, nullptr, &m_Swapchain);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a SwapChain!");
	}

	// Salviamo dei riferimenti relativi al formato e all'extent (servirà nella creazione del RenderPass, TODO)
	m_SwapChainImageFormat = surfaceFormat.format;
	m_SwapChainExtent = extent;

	// Salviamo le Image pre-esistenti all'interno della SwapChain in un vettore
	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(m_VulkanDevice, m_Swapchain, &swapChainImageCount, nullptr);

	std::vector <VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(m_VulkanDevice, m_Swapchain, &swapChainImageCount, images.data());

	// Partendo dalle Image pre-esistenti, costruiamo delle immagini che hanno la stessa Image ed una relativa ImageView
	// e le aggiungiamo al vettore interno al Renderer 'm_swapChainImages'.
	for (VkImage image : images)
	{
		SwapChainImage swapChainImage = {};

		swapChainImage.image = image;
		swapChainImage.imageView = Utility::CreateImageView(m_VulkanDevice, image, m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		m_SwapChainImages.push_back(swapChainImage);
	}
}

// Creazione dei Framebuffer, effettuano una connessione tra il RenderPass e le immagini. Utilizzando rispettivamente Attachments ed ImageView
void SwapChainHandler::CreateFrameBuffers(VkRenderPass &renderPass, VkImageView & depthBufferImageView)
{
	ResizeFrameBuffers();

	// Creiamo un framebuffer per ogni immagine della swapchain
	for (uint32_t i = 0; i < NumOfFrameBuffers(); ++i)
	{
		std::array<VkImageView, 2> attachments = {
			GetSwapChainImageView(i),
			depthBufferImageView
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = renderPass;							   // RenderPass
		frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // Numero degli attachments
		frameBufferCreateInfo.pAttachments = attachments.data();						   // Lista degli attachments 1:1 con il RenderPass
		frameBufferCreateInfo.width  = GetExtentWidth();				   // Framebuffer width
		frameBufferCreateInfo.height = GetExtentHeight();				   // Framebuffer height
		frameBufferCreateInfo.layers = 1;										   // Framebuffer layers

		VkResult result = vkCreateFramebuffer(m_MainDevice.LogicalDevice, &frameBufferCreateInfo, nullptr, &GetFrameBuffer(i));

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Framebuffer!");
		}

	}
}

SwapChainDetails SwapChainHandler::GetSwapChainDetails(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface)
{
	SwapChainDetails swapChainDetails;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainDetails.surfaceCapabilities);

	// Scarica in un'array delle coppie <Formato Immagine, Spazio Colore> che sono supportate dalla Surface
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount > 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapChainDetails.formats.data());
	}

	// Preleva le Presentation Mode supportate dalla Surface
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationCount, nullptr);

	if (presentationCount > 0)
	{
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationCount, swapChainDetails.presentationModes.data());
	}

	return swapChainDetails;
}

VkSurfaceFormatKHR SwapChainHandler::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
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

VkPresentModeKHR SwapChainHandler::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	for (const auto& presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChainHandler::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
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
		glfwGetFramebufferSize(m_GLFWwindow, &width, &height);	// Si prelevano le dimensioni della finestra di GLFW
															// Si crea una nuova Extent con le dimensioni corrette
		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
			std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));

		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
			std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}