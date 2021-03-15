#pragma once

#include <vector>

const bool validationEnabled = true;

// List of validation layers to use
// VK_LAYER_LUNARG_standard_validation = All standard validation layers
const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

// Callback function for validation debugging (will be called when validation information record)
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,				// Type of error
	VkDebugReportObjectTypeEXT objType,			// Type of object causing error
	uint64_t obj,								// ID of object
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* message,						// Validation Information
	void* userData)
{
	// If validation ERROR, then output error and return failure
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		printf("VALIDATION ERROR: %s\n", message);
		return VK_TRUE;
	}

	// If validation WARNING, then output warning and return okay
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		printf("VALIDATION WARNING: %s\n", message);
		return VK_FALSE;
	}

	return VK_FALSE;
}

static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	// vkGetInstanceProcAddr returns a function pointer to the requested function in the requested instance
	// resulting function is cast as a function pointer with the header of "vkCreateDebugReportCallbackEXT"
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

	// If function was found, executre if with given data and return result, otherwise, return error
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	// get function pointer to requested function, then cast to function pointer for vkDestroyDebugReportCallbackEXT
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

	// If function found, execute
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}