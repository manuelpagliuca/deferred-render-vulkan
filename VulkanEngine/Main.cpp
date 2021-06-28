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

	glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, 3.0f);	// Vector in world space that points to camera position
	glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);	// Vector in world space, parallel to the y axis
	Camera camera = Camera(cam_pos, world_up, -60.0f, 0.0f, 5.0f, 0.5f);

	if (vulkanRenderer->Init(&window) == EXIT_FAILURE)
		return EXIT_FAILURE;

	float angle		= 0.0f;
	float delta_time = 0.0f;
	float last_time  = 0.0f;
	
	// Lights Positions
	pcg_extras::seed_seq_from<std::random_device> seed_source;
	pcg32 rng(seed_source);
	std::uniform_real_distribution<float> uniform_dist(-1.0f, 1.0f);

	const float start0 = uniform_dist(rng);
	const float start1 = uniform_dist(rng);
	const float start2 = uniform_dist(rng);

	float lights_pos	= 0.0f;
	float lights_speed	= 0.0001f;


	// Light Colours
	glm::vec3 light_col(1.0f);
	int light_idx = 0;

	// Imgui passing parameters
	GUI::GetInstance()->SetRenderData(
		vulkanRenderer->GetRenderData(), window.getWindow(),
		vulkanRenderer->GetUBOSettingsRef(), &lights_speed, &light_idx, &light_col);
	GUI::GetInstance()->Init();
	GUI::GetInstance()->LoadFontsToGPU();

	// Floor
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(-4.5f, -0.5f, 2.5f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	vulkanRenderer->UpdateModel(3, model);

	// Vivi
	glm::mat4 fst_model(1.0f);
	glm::mat4 snd_model(1.0f);
	glm::mat4 thd_model(1.0f);

	fst_model = glm::translate(fst_model, glm::vec3(0.0f, -0.5f, 0.f));
	fst_model = glm::scale(fst_model, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));

	snd_model = glm::translate(snd_model, glm::vec3(-1.0f, -0.5f, 0.0f));
	snd_model = glm::scale(snd_model, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));

	thd_model = glm::translate(thd_model, glm::vec3(1.0f, -0.5f, 0.0f));
	thd_model = glm::scale(thd_model, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));

	vulkanRenderer->UpdateModel(0, fst_model);
	vulkanRenderer->UpdateModel(1, snd_model);
	vulkanRenderer->UpdateModel(2, thd_model);

	while (!glfwWindowShouldClose(window.getWindow()))
	{
		/* Events */
		glfwPollEvents();
		
		/* Delta Time */
		float const now = static_cast<float const>(glfwGetTime());
		delta_time		= now - last_time;
		last_time		= now;

		/* Camera */
		camera.keyControl(window.getsKeys(), delta_time);
		vulkanRenderer->UpdateCameraPosition(camera.calculateViewMatrix());

		/* Lights Movement */

		vulkanRenderer->UpdateLightPosition(0, glm::vec3(sinf(start0 + lights_pos), sinf(start1 + lights_pos), sinf(start2 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(1, glm::vec3(cosf(start1 + lights_pos), cosf(start2 + lights_pos), cosf(start0 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(2, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(3, glm::vec3(cosf(start0 + lights_pos), cosf(start1 + lights_pos), cosf(start2 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(4, glm::vec3(sinf(start1 + lights_pos), sinf(start2 + lights_pos), sinf(start0 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(5, glm::vec3(cosf(start2 + lights_pos), cosf(start0 + lights_pos), cosf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(6, glm::vec3(cosf(start0 + lights_pos), cosf(start1 + lights_pos), cosf(start2 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(7, glm::vec3(sinf(start1 + lights_pos), sinf(start2 + lights_pos), sinf(start0 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(8, glm::vec3(cosf(start2 + lights_pos), cosf(start0 + lights_pos), cosf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(9, glm::vec3(cosf(start0 + lights_pos), cosf(start1 + lights_pos), cosf(start2 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(10, glm::vec3(sinf(start1 + lights_pos), sinf(start2 + lights_pos), sinf(start0 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(11, glm::vec3(cosf(start2 + lights_pos), cosf(start0 + lights_pos), cosf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(12, glm::vec3(cosf(start0 + lights_pos), cosf(start1 + lights_pos), cosf(start2 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(13, glm::vec3(sinf(start1 + lights_pos), sinf(start2 + lights_pos), sinf(start0 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(14, glm::vec3(cosf(start2 + lights_pos), cosf(start0 + lights_pos), cosf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(15, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(16, glm::vec3(cosf(start2 + lights_pos), cosf(start0 + lights_pos), cosf(start1 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(17, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));

		vulkanRenderer->UpdateLightPosition(18, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));
		vulkanRenderer->UpdateLightPosition(19, glm::vec3(sinf(start2 + lights_pos), sinf(start0 + lights_pos), sinf(start1 + lights_pos)));

		lights_pos += (lights_speed / 10000.0f);

		/* Lights Colour */
		vulkanRenderer->UpdateLightColour(light_idx, light_col);

		/* imgui */
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