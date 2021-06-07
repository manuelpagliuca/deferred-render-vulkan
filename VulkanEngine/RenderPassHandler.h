#pragma once

#include "pch.h"

#include "Utilities.h"
#include "SwapChainHandler.h"

class RenderPassHandler
{
public:
	RenderPassHandler();
	RenderPassHandler(MainDevice *maindevice, SwapChain* swapChainHandler);

	VkRenderPass* GetOffScreenRenderPassReference() { return &m_OffScreenRenderPass; }
	VkRenderPass& GetOffScreenRenderPass()		    { return m_OffScreenRenderPass; }
	VkRenderPass* GetRenderPassReference() { return &m_RenderPass; }
	VkRenderPass& GetRenderPass()		   { return m_RenderPass; }

	void CreateOffScreenRenderPass();
	void CreateRenderPass();
	VkAttachmentDescription SwapchainColourAttachment(const VkFormat& imageFormat);
	VkAttachmentDescription InputPositionAttachment(const VkFormat& imageFormat);
	VkAttachmentDescription InputColourAttachment(const VkFormat& imageFormat);
	VkAttachmentDescription InputDepthAttachment();

	//void SetSubpassDescription();
	std::array<VkSubpassDependency, 2> SetSubpassDependencies();
	
	void DestroyRenderPass();

private:
	MainDevice				*m_MainDevice;
	SwapChain				*m_SwapChainHandler;
	VkRenderPass			m_RenderPass = {};
	VkRenderPass			m_OffScreenRenderPass = {};
	

	//VkAttachmentDescription m_ColourAttachment = {};
	//VkAttachmentDescription m_InputAttachment  = {};
	//VkAttachmentDescription m_DepthAttachment  = {};

	//VkAttachmentReference	m_ColourAttachmentReference = {};
	//VkAttachmentReference	m_DepthAttachmentReference	= {};

	//VkSubpassDescription	m_SubpassDescription = {};
	//std::array<VkSubpassDependency, 2> m_SubpassDependencies = {};
};