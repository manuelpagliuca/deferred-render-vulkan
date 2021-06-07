#pragma once

#include "pch.h"

class Descriptors
{
public:
	Descriptors();
	Descriptors(VkDevice* device);

	void CreateDescriptorPools(size_t swapchain_images, size_t vp_ubo_size, size_t light_ubo_size);
	void CreateSetLayouts();

	void CreateViewProjectionDescriptorSets(const std::vector<VkBuffer>& view_projection_ubo, size_t data_size, size_t swapchain_images);
	void CreateInputAttachmentsDescriptorSets(size_t swapchain_size, const std::vector<BufferImage>& position_buffer, 
		const std::vector<BufferImage>& color_buffer, const std::vector<BufferImage>& normal_buffer);
	void CreateLightDescriptorSets(const std::vector<VkBuffer>& ubo_light, size_t data_size, size_t swapchain_images);

	VkDescriptorSetLayout& GetViewProjectionSetLayout();
	VkDescriptorSetLayout& GetTextureSetLayout();
	VkDescriptorSetLayout& GetInputSetLayout();
	VkDescriptorSetLayout& GetLightSetLayout();
	
	VkDescriptorPool& GetVpPool();
	VkDescriptorPool& GetImguiDescriptorPool();
	VkDescriptorPool& GetTexturePool();
	VkDescriptorPool& GetInputPool();
	VkDescriptorPool& GetLightPool();

	std::vector<VkDescriptorSet>& GetDescriptorSets();
	std::vector<VkDescriptorSet>& GetInputDescriptorSets();
	std::vector<VkDescriptorSet>& GetLightDescriptorSets();

	void DestroyTexturePool();
	void DestroyViewProjectionPool();
	void DestroyImguiPool();
	void DestroyInputPool();
	void DestroyLightPool();

	void DestroyTextureLayout();
	void DestroyViewProjectionLayout();
	void DestroyInputAttachmentsLayout();
	void DestroyLightLayout();
	
private:
	void CreateViewProjectionPool(size_t swapchain_images, size_t ubo_size);
	void CreateTexturePool();
	void CreateImguiPool();
	void CreateInputAttachmentsPool(size_t swapchain_images);
	void CreateLightPool(size_t swapchain_images, size_t ubo_size);

	void CreateViewProjectionSetLayout();
	void CreateTextureSetLayout();
	void CreateInputSetLayout();
	void CreateLightSetLayout();

private:
	VkDevice *m_Device;

	VkDescriptorPool	m_ViewProjectionPool;
	VkDescriptorPool	m_TexturePool;
	VkDescriptorPool	m_InputPool;
	VkDescriptorPool	m_ImguiDescriptorPool;
	VkDescriptorPool	m_LightPool;

	VkDescriptorSetLayout m_ViewProjectionLayout;
	VkDescriptorSetLayout m_TextureLayout;
	VkDescriptorSetLayout m_InputLayout;
	VkDescriptorSetLayout m_LightLayout;

	std::vector<VkDescriptorSet> m_DescriptorSets;
	std::vector<VkDescriptorSet> m_InputDescriptorSets;
	std::vector<VkDescriptorSet> m_LightDescriptorSets;
};