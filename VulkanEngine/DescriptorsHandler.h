#pragma once

#include "pch.h"

class DescriptorsHandler
{
public:
	DescriptorsHandler();
	DescriptorsHandler(VkDevice* device);

	void CreateDescriptorPools(size_t numOfSwapImgs, size_t UBOsize);
	void CreateSetLayouts();

	void CreateDescriptorSets(std::vector<VkBuffer>& viewProjectionUBO, size_t dataSize, size_t numSwapChainImgs);

	VkDescriptorSetLayout& GetViewProjectionDescriptorSetLayout();
	VkDescriptorSetLayout& GetTextureDescriptorSetLayout();
	VkDescriptorPool& GetVpPool();
	VkDescriptorPool& GetImguiDescriptorPool();
	VkDescriptorPool& GetTexturePool();
	std::vector<VkDescriptorSet>& GetDescriptorSets();

	void DestroyTexturePool();
	void DestroyViewProjectionPool();
	void DestroyImguiDescriptorPool();

	void DestroyTextureLayout();
	void DestroyViewProjectionLayout();
	
private:
	void CreateViewProjectionPool(size_t numOfSwapImgs, size_t UBOsize);
	void CreateTexturePool();
	void CreateImguiPool();

	void CreateViewProjectionDescriptorSetLayout();
	void CreateTextureDescriptorSetLayout();

private:
	VkDevice *m_Device;

	VkDescriptorPool	  m_ViewProjectionPool;
	VkDescriptorSetLayout m_ViewProjectionLayout;

	VkDescriptorPool	  m_TexturePool;
	VkDescriptorSetLayout m_TextureLayout;

	VkDescriptorPool m_ImguiDescriptorPool;

	std::vector<VkDescriptorSet> m_DescriptorSets;
};