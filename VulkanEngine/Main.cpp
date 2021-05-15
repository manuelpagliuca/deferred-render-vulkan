#include "pch.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

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

	InitWindow(&window, "Test Vulkan 13/03/2021", 1280, 720); 

	if (vulkanRenderer->Init(window) == EXIT_FAILURE)
		return EXIT_FAILURE;

	float angle		= 0.f;
	float deltaTime = 0.f;
	float lastTime  = 0.f;
	
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance			= vulkanRenderer->GetInstance();;
	init_info.PhysicalDevice	= vulkanRenderer->GetPhysicalDevice();
	init_info.Device			= vulkanRenderer->GetLogicalDevice();
	init_info.PipelineCache		= VK_NULL_HANDLE;
	init_info.Allocator			= nullptr;
	init_info.QueueFamily		= vulkanRenderer->GetQueueFamiliesIndices().GraphicsFamily;
	init_info.Queue				= vulkanRenderer->GetGraphicQueue();
	init_info.DescriptorPool	= vulkanRenderer->GetDescriptorHandler().GetImguiDescriptorPool();
	init_info.MinImageCount		= 3;
	init_info.ImageCount		= 3;
	init_info.CheckVkResultFn	= nullptr;

	ImGui_ImplVulkan_Init(&init_info, vulkanRenderer->GetRenderPassHandler().GetRenderPass());
	
	{
		VkCommandPool command_pool		= vulkanRenderer->GetCommandHandler().GetCommandPool();// wd->Frames[wd->FrameIndex].CommandPool;

		VkCommandBuffer command_buffer	= vulkanRenderer->GetCommandHandler().GetCommandBuffers()[0];//  wd->Frames[wd->FrameIndex].CommandBuffer;

		vkResetCommandPool(vulkanRenderer->GetLogicalDevice(), command_pool, 0);
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	|= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(command_buffer, &begin_info);

		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers	= &command_buffer;
		vkEndCommandBuffer(command_buffer);
		vkQueueSubmit(vulkanRenderer->GetGraphicQueue(), 1, &end_info, VK_NULL_HANDLE);
		vkDeviceWaitIdle(vulkanRenderer->GetLogicalDevice());
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		float const now = static_cast<double>(glfwGetTime());
		deltaTime		= now - lastTime;
		lastTime		= now;

		angle += 1.0f * deltaTime*5.0f;
		if (angle > 360.0f)
			angle -= 360.0f;

		glm::mat4 firstModel(1.0f);
		glm::mat4 secondModel(1.0f);

		firstModel	= glm::translate(firstModel, glm::vec3(0.0f, 0.0f, -2.5f));
		firstModel	= glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

		secondModel = glm::translate(secondModel, glm::vec3(0.0f, 0.0f, angle));
		secondModel = glm::rotate(secondModel, glm::radians(-angle * 10), glm::vec3(0.0f, 0.0f, 1.0f));

		vulkanRenderer->UpdateModel(0, firstModel);
		vulkanRenderer->UpdateModel(1, secondModel);
				
		
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::Render();
		
		ImDrawData* draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

		if (!is_minimized)
			vulkanRenderer->Draw(draw_data);

		//vulkanRenderer->Draw(nullptr);
	}

	vulkanRenderer->Cleanup();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}