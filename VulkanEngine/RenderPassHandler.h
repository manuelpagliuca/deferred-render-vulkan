#pragma once

#include "pch.h"

#include "Utilities.h"
#include "SwapChainHandler.h"

class RenderPassHandler
{
public:
	RenderPassHandler();
	RenderPassHandler(MainDevice *maindevice, SwapChainHandler* swapChainHandler);
	
	VkRenderPass* GetRenderPassReference() { return &m_RenderPass; }
	VkRenderPass& GetRenderPass()		   { return m_RenderPass; }

	void CreateRenderPass();
	void SetColourAttachment(VkFormat imageFormat);
	void SetDepthAttachment();
	void SetSubpassDescription();
	void SetSubpassDependencies();
	
	void DestroyRenderPass();

private:
	MainDevice				*m_MainDevice;
	SwapChainHandler		*m_SwapChainHandler;
	VkRenderPass			m_RenderPass = {};

	VkAttachmentDescription m_ColourAttachment = {};
	VkAttachmentDescription m_DepthAttachment  = {};

	VkAttachmentReference	m_ColourAttachmentReference = {};
	VkAttachmentReference	m_DepthAttachmentReference	= {};

	VkSubpassDescription	m_SubpassDescription = {};
	std::array<VkSubpassDependency, 2> m_SubpassDependencies = {};
};