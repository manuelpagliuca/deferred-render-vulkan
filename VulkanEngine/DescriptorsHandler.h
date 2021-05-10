#pragma once

#include "pch.h"

class DescriptorsHandler
{
public:
	DescriptorsHandler();
	DescriptorsHandler(VkDevice* device);

	void CreateDescriptorPool(size_t numOfSwapImgs, size_t UBOsize);
	void CreateViewProjectionDescriptorSetLayout();
	void CreateTextureDescriptorSetLayout();

	void CreateDescriptorSets(std::vector<VkBuffer>& viewProjectionUBO, size_t dataSize, size_t numSwapChainImgs);

	VkDescriptorSetLayout& GetViewProjectionDescriptorSetLayout();
	VkDescriptorSetLayout& GetTextureDescriptorSetLayout();
	VkDescriptorPool& GetTexturePool();
	std::vector<VkDescriptorSet>& GetDescriptorSets();

	void DestroyTexturePool();
	void DestroyTextureLayout();
	void DestroyViewProjectionPool();
	void DestroyViewProjectionLayout();

private:
	VkDevice *m_Device;

	VkDescriptorPool	  m_ViewProjectionPool;
	VkDescriptorSetLayout m_ViewProjectionLayout;

	VkDescriptorPool	  m_TexturePool;
	VkDescriptorSetLayout m_TextureLayout;

	std::vector<VkDescriptorSet> m_DescriptorSets;

};