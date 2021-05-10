#include "pch.h"

#include "VulkanRenderer.h"

void InitWindow(GLFWwindow** t_window, std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	int res = glfwInit();

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
	VulkanRenderer* vulkanRenderer = new VulkanRenderer();
	GLFWwindow* window = nullptr;

	InitWindow(&window, "Test Vulkan 13/03/2021", 1920, 1080); 

	if (vulkanRenderer->Init(window) == EXIT_FAILURE)
		return EXIT_FAILURE;

	float angle		= 0.f;
	float deltaTime = 0.f;
	float lastTime  = 0.f;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 1.0f * deltaTime*5.0f;
		if (angle > 360.0f) { angle -= 360.0f; }

		glm::mat4 firstModel(1.0f);
		glm::mat4 secondModel(1.0f);

		firstModel = glm::translate(firstModel, glm::vec3(0.0f, 0.0f, -2.5f));
		firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

		secondModel = glm::translate(secondModel, glm::vec3(0.0f, 0.0f, angle));
		secondModel = glm::rotate(secondModel, glm::radians(-angle * 10), glm::vec3(0.0f, 0.0f, 1.0f));

		vulkanRenderer->UpdateModel(0, firstModel);
		vulkanRenderer->UpdateModel(1, secondModel);

		vulkanRenderer->Draw();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}