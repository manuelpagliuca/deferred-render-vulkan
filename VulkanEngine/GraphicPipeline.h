#pragma once

#include "Utilities.h"
#include "RenderPassHandler.h"
#include "SwapChainHandler.h"

class GraphicPipeline
{
public:
	GraphicPipeline();
	GraphicPipeline(MainDevice* main_device, SwapChain* swap_chain, RenderPassHandler* render_pass_handler);

	VkPipeline&		GetPipeline()   { return m_FirstPipeline; }
	VkPipeline&		GetSecondPipeline() { return m_SecondPipeline; }

	VkPipelineLayout& GetLayout()		{ return m_FirstPipelineLayout; }
	VkPipelineLayout& GetSecondLayout()	{ return m_SecondPipelineLayout; }

	VkShaderModule CreateShaderModules(const char* path);
	VkPipelineShaderStageCreateInfo CreateVertexShaderStage(const char* vert_str);
	VkPipelineShaderStageCreateInfo CreateFragmentShaderStage(const char* frag_str);

	void SetDescriptorSetLayouts(
		VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSetLayout& textureObjects,
		VkDescriptorSetLayout& inputSetLayout, VkDescriptorSetLayout& light_set_layout);
	void SetPushCostantRange(VkPushConstantRange& pushCostantRange);
	void SetVertexStageBindingDescription();
	void SetVertexttributeDescriptions();
	void SetViewport();
	void SetScissor();

	void CreateGraphicPipeline();
	void CreateVertexInputStage();
	void CreateInputAssemblyStage();
	void CreateViewportScissorStage();
	void CreateDynamicStatesStage();
	void CreateRasterizerStage();
	void CreateMultisampleStage();
	void CreateColourBlendingStage();
	void CreateDepthStencilStage();

	void DestroyShaderModules();
	void DestroyPipeline();

private:
	MainDevice				*m_MainDevice;
	SwapChain				*m_SwapChain;
	RenderPassHandler		*m_RenderPassHandler;

	VkDescriptorSetLayout	m_ViewProjectionSetLayout;
	VkDescriptorSetLayout	m_TextureSetLayout;
	VkDescriptorSetLayout	m_InputSetLayout;
	VkDescriptorSetLayout	m_LightSetLayout;
	VkPushConstantRange		m_PushCostantRange;

private:
	VkPipeline		m_FirstPipeline;
	VkPipeline		m_SecondPipeline;

	VkPipelineLayout  m_FirstPipelineLayout;
	VkPipelineLayout  m_SecondPipelineLayout;
		
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