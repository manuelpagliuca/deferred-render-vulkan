#pragma once

#include "Utilities.h"
#include "RenderPassHandler.h"
#include "SwapChainHandler.h"

class GraphicPipeline
{
public:
	GraphicPipeline();
	GraphicPipeline(MainDevice &mainDevice, SwapChainHandler& swapChainHandler, RenderPassHandler& renderPass, VkDescriptorSetLayout& descriptorSetLayout,
		VkDescriptorSetLayout& textureObjects, VkPushConstantRange& pushCostantRange);

	VkPipeline&		  GetPipeline()   { return m_GraphicsPipeline; }
	VkPipelineLayout& GetLayout()	  { return m_PipelineLayout; }

	VkShaderModule CreateShaderModules(const char* path);
	VkPipelineShaderStageCreateInfo CreateVertexShaderStage();
	VkPipelineShaderStageCreateInfo CreateFragmentShaderStage();

	void SetVertexStageBindingDescription();
	void SetVertexttributeDescriptions();
	void SetViewport();
	void SetScissor();

	void CreateGraphicPipeline();
	void CreateShaderStages();
	void CreateVertexInputStage();
	void CreateInputAssemblyStage();
	void CreateViewportScissorStage();
	void CreateDynamicStatesStage();
	void CreateRasterizerStage();
	void CreateMultisampleStage();
	void CreateColourBlendingStage();
	void CreateDepthStencilStage();
	void CreatePipelineLayout();

	void DestroyShaderModules();
	void DestroyPipeline();

private:
	MainDevice			  m_MainDevice;
	SwapChainHandler	  m_SwapChainHandler;
	RenderPassHandler	  m_RenderPassHandler;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorSetLayout m_TextureSetLayout;
	VkPushConstantRange   m_PushCostantRange;

	const char* m_VertexShaderSPIRVPath   = "./Shaders/vert.spv";
	const char* m_FragmentShaderSPIRVPath = "./Shaders/frag.spv";

private:
	VkPipeline		  m_GraphicsPipeline;
	VkPipelineLayout  m_PipelineLayout;
	
	VkPipelineShaderStageCreateInfo		   m_ShaderStages[2]	  = {};
	VkPipelineVertexInputStateCreateInfo   m_VertexInputStage	  = {};
	VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyStage	  = {};
	VkPipelineViewportStateCreateInfo	   m_ViewportScissorStage = {};
	VkPipelineRasterizationStateCreateInfo m_Rasterizer			  = {};
	VkPipelineMultisampleStateCreateInfo   m_MultisampleStage	  = {};
	VkPipelineColorBlendStateCreateInfo	   m_ColourBlendingStage  = {};
	VkPipelineDepthStencilStateCreateInfo  m_DepthStencilStage    = {};

private:
	VkShaderModule m_VertexShaderModule;										
	VkShaderModule m_FragmentShaderModule;									

	VkVertexInputBindingDescription					 m_VertexStageBindingDescription = {};				
	std::array<VkVertexInputAttributeDescription, 3> m_VertexStageAttributeDescriptions;

	VkRect2D   m_Scissor  = {};
	VkViewport m_Viewport = {};

	VkPipelineColorBlendAttachmentState m_ColourStateAttachment = {};
};