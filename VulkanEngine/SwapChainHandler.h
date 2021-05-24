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

class SwapChain
{
public:
	SwapChain();
	SwapChain(MainDevice& mainDevice, VkSurfaceKHR& surface, GLFWwindow* glfwWindow, QueueFamilyIndices& queueFamilyIndices);

	/* Generic */
	void CreateSwapChain();
	void RecreateSwapChain();
	void CreateFrameBuffers(VkImageView& depthBufferImageView);
	void CleanUpSwapChain();
	void DestroyFrameBuffers();
	void DestroySwapChainImageViews();
	void DestroySwapChain();

	/* Getters */
	VkSwapchainKHR* GetSwapChainData();
	VkSwapchainKHR& GetSwapChain();
	uint32_t GetExtentWidth() const;
	uint32_t GetExtentHeight() const;
	VkExtent2D& GetExtent();
	VkFormat& GetSwapChainImageFormat();
	SwapChainDetails GetSwapChainDetails(VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface);

	/* Setters */
	void SetRenderPass(VkRenderPass* renderPass);
	void SetRecreationStatus(bool const status);

	/* Vectors operations */
	std::vector<VkFramebuffer>& GetFrameBuffers();
	size_t SwapChainImagesSize() const;
	size_t FrameBuffersSize() const;
	SwapChainImage* GetImage(uint32_t index);
	VkImageView& GetSwapChainImageView(uint32_t index);
	VkFramebuffer& GetFrameBuffer(uint32_t index);
	void PushImage(SwapChainImage swapChainImge);
	void PushFrameBuffer(VkFramebuffer frameBuffer);
	void ResizeFrameBuffers();
	
private:
	/* References of the renderer */
	MainDevice			m_MainDevice;
	VkSurfaceKHR		m_VulkanSurface;
	GLFWwindow*			m_GLFWwindow;
	QueueFamilyIndices  m_QueueFamilyIndices;

	VkRenderPass*		m_RenderPass;

	/* Kernel Of the SwapChainHandler*/
	private:
	VkSwapchainKHR m_Swapchain;
	std::vector<SwapChainImage> m_SwapChainImages;
	std::vector<VkFramebuffer>  m_SwapChainFrameBuffers;

	VkFormat	 m_SwapChainImageFormat;
	VkExtent2D	 m_SwapChainExtent;

	bool m_IsRecreating = false;

private:
	VkSurfaceFormatKHR  ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR	ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D			ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
};