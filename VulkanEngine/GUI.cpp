#include "pch.h"
#include "GUI.h"

GUI* GUI::s_Instance = nullptr;

void GUI::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(m_Window, true);
	init_info.Instance			= m_Data.instance;
	init_info.PhysicalDevice	= m_Data.physical_device;
	init_info.Device			= m_Data.device;
	init_info.PipelineCache		= VK_NULL_HANDLE;
	init_info.Allocator			= nullptr;
	init_info.QueueFamily		= m_Data.graphic_queue_index;
	init_info.Queue				= m_Data.graphic_queue;
	init_info.DescriptorPool	= m_Data.imgui_descriptor_pool;
	init_info.MinImageCount		= m_Data.min_image_count;
	init_info.ImageCount		= m_Data.image_count;
	init_info.CheckVkResultFn	= nullptr;
}

void GUI::LoadFontsToGPU()
{
	ImGui_ImplVulkan_Init(&init_info, m_Data.render_pass);

	VkCommandPool command_pool = m_Data.command_pool;

	VkCommandBuffer command_buffer = m_Data.command_buffers[0];

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
	vkQueueSubmit(m_Data.graphic_queue, 1, &end_info, VK_NULL_HANDLE);
	vkDeviceWaitIdle(m_Data.device);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void GUI::Render()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGuiWindowFlags window_flags{ ImGuiWindowFlags_NoResize };
	ImGui::SetNextWindowSize(ImVec2(350.f, 480.f), 0);
	ImGui::SetNextWindowPos(ImVec2(50.f, 50.f), ImGuiCond_FirstUseEver);
	
	ImGui::Begin("Settings", NULL, window_flags);
	ImGui::Combo("Render Target", &m_SettingsData->render_target, "Position\0Normals\0Albedo\0Deferred Rendering\0");

	switch (m_SettingsData->render_target)
	{
	case 0:
		m_SettingsData->render_target = 0;
		break;
	case 1:
		m_SettingsData->render_target = 1;
		break;
	case 2:
		m_SettingsData->render_target = 2;
		break;
	case 3:
		m_SettingsData->render_target = 3;
		break;
	}

	ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "The lights are enabled only \nwhen using deferred rendering");

	ImGui::SliderFloat("Lights Movement Speed", m_LightsSpeed, 0.0f, 10.0f);
	

	ImGui::Combo("Select Light", m_LightIdx, 
		"1\0""2\0""3\0""4\0""5\0"
		"6\0""7\0""8\0""9\0""10\0"
		"11\0""12\0""13\0""14\0""15\0"
		"16\0""17\0""18\0""19\0""20\0");

	ImGui::ColorPicker3("Lights colour", m_Col);
	m_LightCol->r = m_Col[0];
	m_LightCol->g = m_Col[1];
	m_LightCol->b = m_Col[2];

	ImGui::End();
	ImGui::Render();
}

ImDrawData* GUI::GetDrawData()
{
	return ImGui::GetDrawData();
}

void GUI::KeysControl(bool* keys)
{
	if (keys[GLFW_KEY_1])
	{
		m_SettingsData->render_target = 0;
	}

	if (keys[GLFW_KEY_2])
	{
		m_SettingsData->render_target = 1;
	}

	if (keys[GLFW_KEY_3])
	{
		m_SettingsData->render_target = 2;
	}

	if (keys[GLFW_KEY_4])
	{
		m_SettingsData->render_target = 3;
	}
}

void GUI::Destroy()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}