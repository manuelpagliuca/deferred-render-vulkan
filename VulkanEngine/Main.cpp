#include "pch.h"

#include "Window.h"
#include "GUI.h"
#include "VulkanRenderer.h"
#include "Camera.h"

static VulkanRenderer* vulkanRenderer = new VulkanRenderer();

int main(void)
{
	Window window(1280, 720, "Vulkan | Deferred Render");

	if (window.Initialise() == -1)
	{
		return EXIT_FAILURE;
	}

	glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);		// Vector in world space that points to camera position
	glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);			// Vector in world space, parallel to the y axis
	Camera camera = Camera(cameraPosition, worldUp, -60.0f, 0.0f, 5.0f, 0.5f);

	if (vulkanRenderer->Init(&window) == EXIT_FAILURE)
		return EXIT_FAILURE;

	float angle		= 0.0f;
	float deltaTime = 0.0f;
	float lastTime  = 0.0f;
	
	pcg_extras::seed_seq_from<std::random_device> seed_source;

	// Make a random number engine
	pcg32 rng(seed_source);
	std::uniform_real_distribution<float> uniform_dist(-1.0f, 1.0f);

	const float start0 = uniform_dist(rng);
	const float start1 = uniform_dist(rng);
	const float start2 = uniform_dist(rng);

	float lights_pos = 0.0f;
	float lights_speed = 0.0001f;
	GUI::GetInstance()->SetRenderData(vulkanRenderer->GetRenderData(), window.getWindow(),
		vulkanRenderer->GetUBOSettingsRef(), &lights_speed);
	GUI::GetInstance()->Init();
	GUI::GetInstance()->LoadFontsToGPU();

	// floor
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(-4.5f, -0.5f, 2.5f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	vulkanRenderer->UpdateModel(3, model);

	while (!glfwWindowShouldClose(window.getWindow()))
	{
		glfwPollEvents();
		
		float const now = static_cast<float const>(glfwGetTime());
		deltaTime		= now - lastTime;
		lastTime		= now;

		camera.keyControl(window.getsKeys(), deltaTime);
		camera.mouseControl(window.getXChange(), window.getYChange());


		angle += 1.0f * deltaTime*5.0f;
		if (angle > 360.0f)
			angle -= 360.0f;

		glm::mat4 firstModel(1.0f);
		glm::mat4 secondModel(1.0f);
		glm::mat4 thirdModel(1.0f);
		
		firstModel	= glm::translate(firstModel, glm::vec3(0.0f, -0.5f, 0.f));
		firstModel	= glm::scale(firstModel, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));
		
		secondModel = glm::translate(secondModel, glm::vec3(-1.0f, -0.5f, 0.0f));
		secondModel = glm::scale(secondModel, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));

		thirdModel = glm::translate(thirdModel, glm::vec3(1.0f, -0.5f, 0.0f));
		thirdModel = glm::scale(thirdModel, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));
		
		vulkanRenderer->UpdateModel(0, firstModel);
		vulkanRenderer->UpdateModel(1, secondModel);
		vulkanRenderer->UpdateModel(2, thirdModel);

		vulkanRenderer->UpdateCameraPosition(camera.calculateViewMatrix());

		vulkanRenderer->UpdateLightPosition(0, glm::vec3(sinf(start0 + lights_pos), sinf(start1 + lights_pos), sinf(start2 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(1, glm::vec3(sinf(start1 + lights_pos), sinf(start2 + lights_pos), sinf(start0 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(2, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(3, glm::vec3(sinf(start0 + lights_pos), sinf(start1 + lights_pos), sinf(start2 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(4, glm::vec3(sinf(start1 + lights_pos), sinf(start2 + lights_pos), sinf(start0 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(5, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(6, glm::vec3(sinf(start0 + lights_pos), sinf(start1 + lights_pos), sinf(start2 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(7, glm::vec3(sinf(start1 + lights_pos), sinf(start2 + lights_pos), sinf(start0 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(8, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(9, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));

		lights_pos += lights_speed;

		GUI::GetInstance()->Render();
		GUI::GetInstance()->KeysControl(window.getsKeys());

		auto draw_data = GUI::GetInstance()->GetDrawData();

		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

		if (!is_minimized)
			vulkanRenderer->Draw(draw_data);
	}

	vulkanRenderer->Cleanup();

	glfwDestroyWindow(window.getWindow());
	glfwTerminate();

	return 0;
}