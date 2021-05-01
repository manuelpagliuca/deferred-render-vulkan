#pragma once

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>

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
	SwapChainHandler(VkPhysicalDevice& physicalDevice, VkDevice& device, VkSurfaceKHR& surface, GLFWwindow* glfwWindow);

	void CreateSwapChain(QueueFamilyIndices& t_QueueFamilyIndices);

	VkSwapchainKHR* GetSwapChainData();
	VkSwapchainKHR& GetSwapChain();

	void PushImage(SwapChainImage swapChainImge);
	void PushFrameBuffer(VkFramebuffer frameBuffer);

	SwapChainImage* GetImage(uint32_t index);
	VkImageView&	GetSwapChainImageView(uint32_t index);
	VkFramebuffer&  GetFrameBuffer(uint32_t index);
	
	size_t NumOfSwapChainImages() const;
	size_t NumOfFrameBuffers() const;
	void ResizeFrameBuffers();

	uint32_t GetExtentWidth() const;
	uint32_t GetExtentHeight() const;
	VkExtent2D& GetExtent();

	void DestroyFrameBuffers(VkDevice device);
	void DestroySwapChainImages(VkDevice device);
	void DestroySwapChain(VkDevice device);

	VkFormat& GetSwapChainImageFormat();

	SwapChainDetails GetSwapChainDetails(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface);
	
private:
	VkSwapchainKHR m_Swapchain;
	std::vector<SwapChainImage> m_SwapChainImages;		 // Vettore contenente le Image e ImageView della SwapChain
	std::vector<VkFramebuffer>  m_SwapChainFrameBuffers; //
	
	VkFormat	 m_SwapChainImageFormat;   // Formato da utilizzare per l'Image View (prelevato dalla creazione della SwapChain)
	VkExtent2D	 m_SwapChainExtent;		   // Extent da utilizzare per l'Image View (prelevato dalla creazione della SwapChain)

private:
	VkPhysicalDevice m_VulkanPhysicalDevice;
	VkDevice		 m_VulkanDevice;
	VkSurfaceKHR	 m_VulkanSurface;
	GLFWwindow*		 m_GLFWwindow;

	VkSurfaceFormatKHR  ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR	ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D			ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
};

