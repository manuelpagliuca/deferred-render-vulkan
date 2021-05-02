#pragma once

#include "pch.h"

#include "Utilities.h"
#include "SwapChainHandler.h"

class RenderPassHandler
{
public:
	RenderPassHandler();
	RenderPassHandler(MainDevice& maindevice, SwapChainHandler& swapChainHandler);

	void CreateRenderPass(MainDevice& maindevice, SwapChainHandler& swapChainHandler);

	VkRenderPass& GetRenderPass() { return m_RenderPass; }
private:
	VkRenderPass m_RenderPass;
};

