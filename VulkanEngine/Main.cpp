#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.h"

VulkanRenderer* vulkanRenderer = new VulkanRenderer();
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

		float const now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime  = now;
		angle += 10.f * deltaTime;

		if (angle > 360.f)
		{
			angle -= 360.f;
		}

		vulkanRenderer->updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.f, 0.f, 1.0f)));

		vulkanRenderer->draw();
	}

	glfwDestroyWindow(window);				// Distruzione della finestra GLFW
	glfwTerminate();

	return 0;
}