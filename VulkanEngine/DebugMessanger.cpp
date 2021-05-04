#include "pch.h"

#include "DebugMessanger.h"

DebugMessanger* DebugMessanger::s_Instance = nullptr;

void DebugMessanger::Clear()
{
	DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
}

DebugMessanger* DebugMessanger::GetInstance()
{
	if (s_Instance == 0)
	{
		s_Instance = new DebugMessanger();
	}

	return s_Instance;
}

void DebugMessanger::SetupDebugMessenger(VkInstance &instance)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;	// Il tipo della struttura
	createInfo.flags = 0;
	createInfo.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |			// Imposta i tipi di severità a cui siamo interessati
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType = //VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT	|			// Imposta il tipo di messaggio disponibili
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = DebugCallback;	// Imposta la funzione di callback da chiamare in caso di errore
	createInfo.pUserData = nullptr;  		// Dati extra da passare all'interno della callback

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) // Creo il debug messanger, in caso di errore alzo un eccezione
	{
		throw std::runtime_error("Failed to set up debug messenger!");
	}
}


// Funzione di Callback del DebugMessanger
VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessanger::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	switch (messageType)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		std::cerr << "-[General]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		std::cerr << "-[Validation]";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		std::cerr << "-[Performance]";
		break;
	}

	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		std::cerr << " [Warning]";
		break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		std::cerr << " [Verbose]";
		break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		std::cerr << " [Error]";
		break;
	}

	std::cerr << pCallbackData->pMessage << "\n\n";

	return VK_FALSE;
}

// Alloca il DebugMessanger (preleva il comando dall'istanza di Vulkan)
VkResult DebugMessanger::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)						// Le informazioni contenute nella createInfo devono essere passate alla funzione
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");	// "vkCreateDebugUtilsMessengerEXT", ma per sua natura (extension function) non è caricata
																			// nell'API. Si utilizza "vkGetInstanceProcAddr(...)" per recuperarne l'indirizzo e crearne una proxy function.
	if (func != nullptr)
	{
		VkResult result = func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		return result;
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// Distrugge il DebugMessanger (preleva il comando dall'istanza di Vulkan)
void DebugMessanger::DestroyDebugUtilsMessengerEXT(
	VkInstance &instance,
	VkDebugUtilsMessengerEXT &m_debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{

	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)						// Per distruggere il debug messanger si deve utilizzare una funzione
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");	// che non è esposta nelle API, "vkDestroyDebugUtilsMessengerEXT".

	if (func != nullptr)
	{
		func(instance, m_debugMessenger, pAllocator);
	}
}
