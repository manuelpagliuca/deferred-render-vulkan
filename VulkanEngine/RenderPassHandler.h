#pragma once

#include "pch.h"

#include "Utilities.h"
#include "SwapChainHandler.h"

class RenderPassHandler
{
public:
	RenderPassHandler();
	RenderPassHandler(MainDevice& maindevice, SwapChainHandler& swapChainHandler);
	
	VkRenderPass& GetRenderPass() { return m_RenderPass; }

	void CreateRenderPass(MainDevice& maindevice, SwapChainHandler& swapChainHandler);
	void SetColourAttachment(VkFormat imageFormat);
	void SetDepthAttachment(MainDevice mainDevice);
	void SetSubpassDescription();
	void SetSubpassDependencies();
	
	void DestroyRenderPass();

private:
	VkDevice m_LogicalDevice;
	VkRenderPass			m_RenderPass = {};

	VkAttachmentDescription m_ColourAttachment = {};
	VkAttachmentDescription m_DepthAttachment  = {};

	VkAttachmentReference	m_ColourAttachmentReference = {};
	VkAttachmentReference	m_DepthAttachmentReference	= {};

	VkSubpassDescription	m_SubpassDescription = {};
	std::array<VkSubpassDependency, 2> m_SubpassDependencies = {};
};

