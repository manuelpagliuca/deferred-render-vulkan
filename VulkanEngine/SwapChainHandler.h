#pragma once

#include "pch.h"

#include "Utilities.h"

struct SwapChainImage {
	VkImage image;
	VkImageView imageView;
};

struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentationModes;
};

class SwapChainHandler
{
public:
	SwapChainHandler();
	SwapChainHandler(MainDevice& mainDevice, VkSurfaceKHR& surface, GLFWwindow* glfwWindow, QueueFamilyIndices& queueFamilyIndices);

	void CreateSwapChain();
	void RecreateSwapChain();
	void CreateFrameBuffers(VkImageView& depthBufferImageView);

	VkSwapchainKHR* GetSwapChainData();
	VkSwapchainKHR& GetSwapChain();

	void PushImage(SwapChainImage swapChainImge);
	void PushFrameBuffer(VkFramebuffer frameBuffer);

	void SetRenderPass(VkRenderPass* renderPass);
	void IsRecreating(bool const status);

	SwapChainImage* GetImage(uint32_t index);
	VkImageView&	GetSwapChainImageView(uint32_t index);
	VkFramebuffer&  GetFrameBuffer(uint32_t index);
	std::vector<VkFramebuffer>& GetFrameBuffers();
	
	size_t SwapChainImagesSize() const;
	size_t FrameBuffersSize() const;
	void ResizeFrameBuffers();

	uint32_t GetExtentWidth() const;
	uint32_t GetExtentHeight() const;
	VkExtent2D& GetExtent();

	void CleanUpSwapChain();
	void DestroyFrameBuffers();
	void DestroySwapChainImageViews();
	void DestroySwapChain();

	VkFormat& GetSwapChainImageFormat();

	SwapChainDetails GetSwapChainDetails(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface);
	
private:
	VkSwapchainKHR m_Swapchain;
	std::vector<SwapChainImage> m_SwapChainImages;		 // Vettore contenente le Image e ImageView della SwapChain
	std::vector<VkFramebuffer>  m_SwapChainFrameBuffers; // Vettore contenente i framebuffers
	
	VkFormat	 m_SwapChainImageFormat;   // Formato da utilizzare per l'Image View (prelevato dalla creazione della SwapChain)
	VkExtent2D	 m_SwapChainExtent;		   // Extent da utilizzare per l'Image View (prelevato dalla creazione della SwapChain)

private:
	MainDevice			m_MainDevice;
	VkSurfaceKHR		m_VulkanSurface;
	GLFWwindow*			m_GLFWwindow;
	QueueFamilyIndices  m_QueueFamilyIndices;

	VkRenderPass*		m_RenderPass;

	bool				m_IsRecreating = false;

	VkSurfaceFormatKHR  ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR	ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D			ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
};

