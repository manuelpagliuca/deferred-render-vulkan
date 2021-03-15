#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>

#include "Utilities.h"

#ifdef NDEBUG										// Abilita le Validation Layers in caso di Debug
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

class VulkanRenderer
{
public:
	VulkanRenderer() = default;
	~VulkanRenderer();

	int init(void* newWindow);
	void cleanup();

private:
	GLFWwindow* m_window = nullptr;				// Finestra GLFW
	/* Componenti Vulkan */
	VkInstance m_instance;						// Instanza di Vulkan
	VkDebugUtilsMessengerEXT m_debugMessenger;  // Debug Messanger (creato solo se le Validation Layers sono abilitate)

	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} m_mainDevice;									// Dispositivo principale (logico-fisico)
	 
	QueueFamilyIndices m_queueFamilyIndices;		// Indici delle Queue Families
	VkSurfaceKHR m_surface;							// Surface di Vulkan (interfaccia per la swapchain)
	VkQueue	m_graphicsQueue;						// Puntatore alla Queue Grafica del device logico
	VkQueue	m_presentationQueue;					// Puntatore alla Queue di Presentazione del device logico
	VkSwapchainKHR m_swapchain;						// Puntatore alla SwapChain
	std::vector<SwapChainImage> m_swapChainImages;	// Vettore contenente le Image e ImageView della SwapChain

	/* Variabili ausiliarie */
	VkFormat m_swapChainImageFormat;				// Formato da utilizzare per l'Image View (prelevato dalla creazione della SwapChain)
	VkExtent2D m_swapChainExtent;					// Extent da utilizzare per l'Image View (prelevato dalla creazione della SwapChain)

private:
	// Funzioni per la creazione
	void createInstance();													// Creawebzione dell'istanza di Vulkan
	void loadGlfwExtensions(std::vector<const char*>& instanceExtensions);	// Carica le estensioni di GLFW sul vettore (non controlla se sono supportate)
	void setQueueFamiliesIndices(VkPhysicalDevice device);					// Imposta l'indice delle Queue Families presenti sul Physical Device
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	// Funzioni per l'inizializzazione del Vulkan Renderer
	void getPhysicalDevice();	// Preleva i dispositivi fisici
	void createLogicalDevice();	// Crea il dispositivo logico
	void createSurface();
	void createSwapChain();
	void createGraphicPipeline();

	// Funzioni di controllo
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtension); // Controlla se le estensioni (scaricaete) che si vogliono utilizzare sono supportate da Vulkan.
	bool checkDeviceSuitable(VkPhysicalDevice device);							  // Controllo se il dispositivo fisico è adatto allo scopo del programma (ha una QueueFamily di tipo Graphics)
	bool checkValidationLayerSupport(std::vector<const char*>* validationLayers); // Controllo se le Validation Layer fornite sono supportate da Vulkan.
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);					  // Controllo se il dispositivo fisico supporta le estensioni specificate in 'deviceExtensions' (Utilities.h)

	/* Funzioni ausiliarie */
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspetFlags);
	VkShaderModule createShaderModule(const std::vector<char>& code);

	/* Funzioni di scelta */
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	// Funzioni per il debug
	void setupDebugMessenger();										// Funzione che imposta il debug messanger
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(			// Funzione callback
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	VkResult CreateDebugUtilsMessengerEXT(						// Recupera la funzione per creare il debug messanger
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(							// Recupera la funzione per distruggere il debug messanger
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);
};