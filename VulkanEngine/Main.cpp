#include "pch.h"

#include "GUI.h"

#include "VulkanRenderer.h"

static VulkanRenderer* vulkanRenderer = new VulkanRenderer();

void InitWindow(GLFWwindow** t_window, std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	int res = glfwInit();

	if (!glfwVulkanSupported())
	{
		std::cerr << "GLFW: Vulkan not supported\n" << std::endl;
		return;
	}

	if (!res)
	{
		throw std::runtime_error("Unable to initialize GLFW.");
	}
		
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Impostare GLFW in modo che non lavori con OpenGL
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);	  // Disabilito la resize (va gestita in un suo modo specifico su Vulkan)

	*t_window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main(void)
{
	GLFWwindow* window = nullptr;

	InitWindow(&window, "Test Vulkan 13/03/2021", 1360, 720); 

	if (vulkanRenderer->Init(window) == EXIT_FAILURE)
		return EXIT_FAILURE;

	float angle		= 0.f;
	float deltaTime = 0.f;
	float lastTime  = 0.f;
	
	GUI::GetInstance()->SetRenderData(vulkanRenderer->GetRenderData(), window);
	GUI::GetInstance()->Init();
	GUI::GetInstance()->LoadFontsToGPU();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		float const now = static_cast<float const>(glfwGetTime());
		deltaTime		= now - lastTime;
		lastTime		= now;

		angle += 1.0f * deltaTime*5.0f;
		if (angle > 360.0f)
			angle -= 360.0f;

		glm::mat4 firstModel(1.0f);
		glm::mat4 secondModel(1.0f);
		glm::mat4 thirdModel(1.0f);
		
		firstModel	= glm::translate(firstModel, glm::vec3(0.0f, -0.5f, 0.f));
		firstModel	= glm::scale(firstModel, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));
		firstModel	= glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

		secondModel = glm::translate(secondModel, glm::vec3(-1.0f, -0.5f, 0.0f));
		secondModel = glm::scale(secondModel, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));
		secondModel = glm::rotate(secondModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

		thirdModel = glm::translate(thirdModel, glm::vec3(1.0f, -0.5f, 0.0f));
		thirdModel = glm::scale(thirdModel, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));
		thirdModel = glm::rotate(thirdModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

		vulkanRenderer->UpdateModel(0, firstModel);
		vulkanRenderer->UpdateModel(1, secondModel);
		vulkanRenderer->UpdateModel(2, thirdModel);

		GUI::GetInstance()->Render();
		auto draw_data = GUI::GetInstance()->GetDrawData();
		
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

		if (!is_minimized)
			vulkanRenderer->Draw(draw_data);
	}

	vulkanRenderer->Cleanup();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}