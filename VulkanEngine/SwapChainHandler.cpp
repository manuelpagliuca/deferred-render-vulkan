#include "pch.h"
#include "SwapChainHandler.h"

SwapChain::SwapChain()
{
	m_Swapchain				= 0;
	m_MainDevice			= {};
	m_VulkanSurface			= 0;
	m_SwapChainExtent		= {};
	m_SwapChainImageFormat	= {};
	m_GLFWwindow			= nullptr;
}
	
SwapChain::SwapChain(MainDevice *mainDevice, VkSurfaceKHR *surface,
	GLFWwindow *glfwWindow, QueueFamilyIndices &queueFamilyIndices)
{
	m_Swapchain				= 0;
	m_MainDevice			= mainDevice;
	m_QueueFamilyIndices	= queueFamilyIndices;
	m_VulkanSurface			= surface;
	m_SwapChainExtent		= {};
	m_SwapChainImageFormat	= {};
	m_GLFWwindow			= glfwWindow;
}

VkSwapchainKHR* SwapChain::GetSwapChainData()
{
	return &m_Swapchain;
}

VkSwapchainKHR& SwapChain::GetSwapChain()
{
	return m_Swapchain;
}

SwapChainImage* SwapChain::GetImage(uint32_t index)
{
	return &m_SwapChainImages[index];
}

void SwapChain::PushImage(SwapChainImage swapChainImage)
{
	m_SwapChainImages.push_back(swapChainImage);
}

VkImageView& SwapChain::GetSwapChainImageView(uint32_t index)
{
	return m_SwapChainImages[index].imageView;
}

size_t SwapChain::SwapChainImagesSize() const
{
	return m_SwapChainImages.size();
}

VkFramebuffer& SwapChain::GetFrameBuffer(uint32_t index)
{
	return m_SwapChainFrameBuffers[index];
}

std::vector<VkFramebuffer>& SwapChain::GetFrameBuffers()
{
	return m_SwapChainFrameBuffers;
}

void SwapChain::PushFrameBuffer(VkFramebuffer frameBuffer)
{
	m_SwapChainFrameBuffers.push_back(frameBuffer);
}

void SwapChain::SetRenderPass(VkRenderPass* renderPass)
{
	m_RenderPass = renderPass;
}

void SwapChain::SetRecreationStatus(bool const status)
{
	m_IsRecreating = status;
}

void SwapChain::ResizeFrameBuffers()
{
	m_SwapChainFrameBuffers.resize(m_SwapChainImages.size());
}

size_t SwapChain::FrameBuffersSize() const
{
	return m_SwapChainFrameBuffers.size();
}


void SwapChain::CleanUpSwapChain()
{
	DestroyFrameBuffers();

	DestroySwapChainImageViews();
	DestroySwapChain();
}

void SwapChain::DestroyFrameBuffers()
{
	for (auto framebuffer : m_SwapChainFrameBuffers)
		vkDestroyFramebuffer(m_MainDevice->LogicalDevice, framebuffer, nullptr);
}

void SwapChain::DestroySwapChainImageViews()
{
	for (auto image : m_SwapChainImages)
		vkDestroyImageView(m_MainDevice->LogicalDevice, image.imageView, nullptr);
}

void SwapChain::DestroySwapChain()
{
	vkDestroySwapchainKHR(m_MainDevice->LogicalDevice, m_Swapchain, nullptr);
}

uint32_t SwapChain::GetExtentWidth() const
{
	return m_SwapChainExtent.width;
}

uint32_t SwapChain::GetExtentHeight() const
{
	return m_SwapChainExtent.height;
}

VkExtent2D& SwapChain::GetExtent()
{
	return m_SwapChainExtent;
}

VkFormat& SwapChain::GetSwapChainImageFormat()
{
	return m_SwapChainImageFormat;
}


void SwapChain::CreateSwapChain()
{
	SwapChainDetails swapChainDetails = GetSwapChainDetails(m_MainDevice->PhysicalDevice, *m_VulkanSurface);

	VkExtent2D extent				  = ChooseSwapExtent(swapChainDetails.surfaceCapabilities);
	VkSurfaceFormatKHR surfaceFormat  = ChooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode	  = ChooseBestPresentationMode(swapChainDetails.presentationModes);

	uint32_t imageCount	= swapChainDetails.surfaceCapabilities.minImageCount + 1;	// triple buffering

	// Se il numero massimo di immagini è positivo e questo numero è minore delle immagini che verranno utilizzate
	// Allora significa che il massimo numero di immagini disponibile non basta per supportare il triple-buffering
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
		swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType				= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface				= *m_VulkanSurface;										  // Surface alla quale viene bindata la SwapChain
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

	if (m_QueueFamilyIndices.GraphicsFamily != m_QueueFamilyIndices.PresentationFamily)
	{
		uint32_t queueFamilyIndices[]{
			static_cast<uint32_t>(m_QueueFamilyIndices.GraphicsFamily),
			static_cast<uint32_t>(m_QueueFamilyIndices.PresentationFamily)
		};

		swapChainCreateInfo.imageSharingMode		= VK_SHARING_MODE_CONCURRENT;	
		swapChainCreateInfo.queueFamilyIndexCount	= 2;							
		swapChainCreateInfo.pQueueFamilyIndices		= queueFamilyIndices;			
	}
	else
	{
		swapChainCreateInfo.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE; 
		swapChainCreateInfo.queueFamilyIndexCount	= 1;						   
		swapChainCreateInfo.pQueueFamilyIndices		= nullptr;				  
	}

	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE; 

	VkResult res = vkCreateSwapchainKHR(m_MainDevice->LogicalDevice, &swapChainCreateInfo, nullptr, &m_Swapchain);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a SwapChain!");
	}

	// Salviamo dei riferimenti relativi al formato e all'extent (servirà nella creazione del RenderPass, TODO)
	m_SwapChainImageFormat	= surfaceFormat.format;
	m_SwapChainExtent		= extent;

	// Salviamo le Image pre-esistenti all'interno della SwapChain in un vettore
	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(m_MainDevice->LogicalDevice, m_Swapchain, &swapChainImageCount, nullptr);

	std::vector <VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(m_MainDevice->LogicalDevice, m_Swapchain, &swapChainImageCount, images.data());

	if (!m_IsRecreating)
	{
		for (VkImage image : images)
		{
			SwapChainImage swapChainImage = {};

			swapChainImage.image	 = image;
			swapChainImage.imageView = Utility::CreateImageView(image, m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

			m_SwapChainImages.push_back(swapChainImage);
		}
	}
	else
		for (size_t i = 0; i < m_SwapChainImages.size() ; ++i)
			m_SwapChainImages[i].imageView = 
			Utility::CreateImageView(images[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	
}

void SwapChain::RecreateSwapChain()
{
	vkDeviceWaitIdle(m_MainDevice->LogicalDevice);
	CleanUpSwapChain();
	CreateSwapChain();
}

void SwapChain::CreateFrameBuffers(const VkImageView & depth_buffer, const std::vector<BufferImage> &color_buffer)
{
	ResizeFrameBuffers();

	for (uint32_t i = 0; i < m_SwapChainFrameBuffers.size(); ++i)
	{
		std::array<VkImageView, 3> attachments = {
			m_SwapChainImages[i].imageView,
			color_buffer[i].ImageView,
			depth_buffer
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass		= *m_RenderPass;		
		frameBufferCreateInfo.attachmentCount	= static_cast<uint32_t>(attachments.size()); 
		frameBufferCreateInfo.pAttachments		= attachments.data();						 
		frameBufferCreateInfo.width				= GetExtentWidth();							 
		frameBufferCreateInfo.height			= GetExtentHeight();						 
		frameBufferCreateInfo.layers			= 1;										 

		VkResult result = vkCreateFramebuffer(m_MainDevice->LogicalDevice, &frameBufferCreateInfo, nullptr, &m_SwapChainFrameBuffers[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Framebuffer!");
		}
	}
}

SwapChainDetails SwapChain::GetSwapChainDetails(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface)
{
	SwapChainDetails swapChainDetails;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainDetails.surfaceCapabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount > 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapChainDetails.formats.data());
	}

	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationCount, nullptr);

	if (presentationCount > 0)
	{
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationCount, swapChainDetails.presentationModes.data());
	}
	return swapChainDetails;
}

VkSurfaceFormatKHR SwapChain::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	bool const onlyOneFormat = formats.size() == 1;
	bool const isUndefined	 = formats[0].format == VK_FORMAT_UNDEFINED;

	if (onlyOneFormat && isUndefined)
		// significa che sono disponibili tutti i formati (no restrictions).
		return { VK_FORMAT_R8G8B8A8_UNORM , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const auto& format : formats)
	{
		bool const isFormatRGBA		= format.format == VK_FORMAT_R8G8B8A8_UNORM;
		bool const isFormatBGRA		= format.format == VK_FORMAT_B8G8R8A8_UNORM;
		bool const isColorSpaceSRGB	= format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

		if ((isFormatRGBA || isFormatBGRA) && isColorSpaceSRGB)
			return format;
	}

	return formats[0];
}

VkPresentModeKHR SwapChain::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	for (const auto& presentationMode : presentationModes)
	{
		bool const isMailBox = presentationMode == VK_PRESENT_MODE_MAILBOX_KHR;

		if (isMailBox)
			return presentationMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// Se la corrente Extent presente nella surface ha una
	// dimensione differente dal limite numerico, allora è contenuta la dimensione corretta della finestra

	bool const isExtentCorrect = surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max();

	if (isExtentCorrect)
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
		newExtent.width		= static_cast<uint32_t>(width);
		newExtent.height	= static_cast<uint32_t>(height);

		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));

		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}