#include "pch.h"
#include "GUI.h"

GUI* GUI::s_Instance = nullptr;

void GUI::Init()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(m_Window, true);
	init_info.Instance			= m_Data.instance;
	init_info.PhysicalDevice	= m_Data.physicalDevice;
	init_info.Device			= m_Data.device;
	init_info.PipelineCache		= VK_NULL_HANDLE;
	init_info.Allocator			= nullptr;
	init_info.QueueFamily		= m_Data.graphicQueueIndex;
	init_info.Queue				= m_Data.graphicQueue;
	init_info.DescriptorPool	= m_Data.imguiDescriptorPool;
	init_info.MinImageCount		= m_Data.minImageCount;
	init_info.ImageCount		= m_Data.imageCount;
	init_info.CheckVkResultFn	= nullptr;
}

void GUI::LoadFontsToGPU()
{
	ImGui_ImplVulkan_Init(&init_info, m_Data.renderPass);

	VkCommandPool command_pool = m_Data.commandPool;

	VkCommandBuffer command_buffer = m_Data.commandBuffers[0];

	vkResetCommandPool(m_Data.device, command_pool, 0);
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(command_buffer, &begin_info);

	ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

	VkSubmitInfo end_info = {};
	end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &command_buffer;
	vkEndCommandBuffer(command_buffer);
	vkQueueSubmit(m_Data.graphicQueue, 1, &end_info, VK_NULL_HANDLE);
	vkDeviceWaitIdle(m_Data.device);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void GUI::Render()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();
}

ImDrawData* GUI::GetDrawData()
{
	return ImGui::GetDrawData();
}

void GUI::Destroy()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}