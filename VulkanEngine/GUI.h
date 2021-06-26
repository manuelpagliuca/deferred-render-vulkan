#pragma once

class GUI
{
public:

	static GUI* GetInstance()
	{
		if (!s_Instance)
			s_Instance = new GUI();
		return s_Instance;
	}

	void SetRenderData(VulkanRenderData data, GLFWwindow * window, SettingsData *ubo_settings, float*lights_speed)
	{
		m_Data			= data;
		m_Window		= window;
		m_SettingsData	= ubo_settings;
		m_LightsSpeed	= lights_speed;
	}

	void Init();
	void LoadFontsToGPU();
	void Render();
	ImDrawData* GetDrawData();
	void KeysControl(bool* keys);
	void Destroy();

private:
	GUI() {};
	static GUI *s_Instance;
	GLFWwindow* m_Window = nullptr;

	SettingsData* m_SettingsData = nullptr;
	float *m_LightsSpeed = nullptr;


	VulkanRenderData m_Data;
	ImGui_ImplVulkan_InitInfo init_info = {};
};
