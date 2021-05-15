#include "pch.h"
#include "GUI.h"

void GUI::Destroy()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}