#pragma once

#include "pch.h"

class DescriptorsHandler
{
public:
	DescriptorsHandler();
	DescriptorsHandler(VkDevice* device);

	void CreateDescriptorPools(size_t numOfSwapImgs, size_t UBOsize);
	void CreateSetLayouts();

	void CreateDescriptorSets(const std::vector<VkBuffer>& viewProjectionUBO, size_t dataSize, size_t numSwapChainImgs,
		const std::vector<BufferImage>& color_buffer, const BufferImage& depth_buffer);

	VkDescriptorSetLayout& GetViewProjectionDescriptorSetLayout();
	VkDescriptorSetLayout& GetTextureDescriptorSetLayout();
	VkDescriptorSetLayout& GetInputDescriptorSetLayout();

	VkDescriptorPool& GetVpPool();
	VkDescriptorPool& GetImguiDescriptorPool();
	VkDescriptorPool& GetTexturePool();
	VkDescriptorPool& GetInputPool();

	std::vector<VkDescriptorSet>& GetDescriptorSets();
	std::vector<VkDescriptorSet>& GetInputDescriptorSets();

	void DestroyTexturePool();
	void DestroyViewProjectionPool();
	void DestroyImguiDescriptorPool();
	void DestroyInputDescriptorPool();

	void DestroyTextureLayout();
	void DestroyViewProjectionLayout();
	void DestroyInputSetLayout();
	
private:
	void CreateViewProjectionPool(size_t numOfSwapImgs, size_t UBOsize);
	void CreateTexturePool();
	void CreateImguiPool();
	void CreateInputPool(size_t numOfSwapImgs);

	void CreateViewProjectionDescriptorSetLayout();
	void CreateTextureDescriptorSetLayout();
	void CreateInputDescriptorSetLayout();

private:
	VkDevice *m_Device;

	VkDescriptorPool	m_ViewProjectionPool;
	VkDescriptorPool	m_TexturePool;
	VkDescriptorPool	m_InputPool;
	VkDescriptorPool	m_ImguiDescriptorPool;

	VkDescriptorSetLayout m_ViewProjectionLayout;
	VkDescriptorSetLayout m_TextureLayout;
	VkDescriptorSetLayout m_InputLayout;

	std::vector<VkDescriptorSet> m_DescriptorSets;
	std::vector<VkDescriptorSet> m_InputDescriptorSets;
};