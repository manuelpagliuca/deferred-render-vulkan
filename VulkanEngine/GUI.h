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

	void SetRenderData(VulkanRenderData data, GLFWwindow * window)
	{
		m_Data	 = data;
		m_Window = window;
	}

	void Init();
	void LoadFontsToGPU();
	void Render();
	ImDrawData* GetDrawData();

	void Destroy();

private:
	GUI() {};
	static GUI *s_Instance;
	GLFWwindow* m_Window;
	
	VulkanRenderData m_Data;
	ImGui_ImplVulkan_InitInfo init_info = {};
};
