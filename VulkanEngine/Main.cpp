#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include "VulkanRenderer.h"

VulkanRenderer *vulkanRenderer = new VulkanRenderer();
GLFWwindow* window			   = nullptr;

// Creazione della finestra GLFW
void initWindow(GLFWwindow** t_window, std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	int res = glfwInit();

	if (!res)
	{
		throw std::runtime_error("Unable to initialize GLFW.");
	}
		
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Impostare GLFW in modo che non lavori con OpenGL
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);	  // Effettuare una resize su una finestra con Vulkan comporta la ricreazione da capo di molti parametri
												  // Quindi viene disabilitata questa feature.

	*t_window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);	// Carico sul puntatore creato
}

int main()
{
	initWindow(&window, "Test Vulkan 13/03/2021", 1024, 720); // Inizializzazione di una finestra GLFW

	if (vulkanRenderer->init(window) == EXIT_FAILURE)		  // Creazione e controllo del VulkanRenderer
	{
		return EXIT_FAILURE;
	}

	float angle		= 0.f;
	float deltaTime = 0.f;
	float lastTime  = 0.f;

	while (!glfwWindowShouldClose(window))	// Loop finchè non si chiude la finestras
	{
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10.0f * deltaTime;
		if (angle > 360.0f) { angle -= 360.0f; }

		glm::mat4 firstModel(1.0f);
		glm::mat4 secondModel(1.0f);

		firstModel = glm::translate(firstModel, glm::vec3(0.0f, 0.0f, -2.5f));
		firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

		secondModel = glm::translate(secondModel, glm::vec3(0.0f, 0.0f, -3.0f));
		secondModel = glm::rotate(secondModel, glm::radians(-angle * 10), glm::vec3(0.0f, 0.0f, 1.0f));

		vulkanRenderer->updateModel(0, firstModel);
		vulkanRenderer->updateModel(1, secondModel);

		vulkanRenderer->draw();
	}

	glfwDestroyWindow(window);				// Distruzione della finestra GLFW
	glfwTerminate();

	return 0;
}