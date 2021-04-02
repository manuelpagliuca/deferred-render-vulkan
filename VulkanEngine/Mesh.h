#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhysicalDevice,
		 VkDevice newLogicalDevice, 
		 VkQueue transferQueue, 
		 VkCommandPool transferCommandPool, 
		 std::vector<Vertex>* vertices,
		 std::vector<uint32_t>* indices);
	~Mesh();

	int		 getVertexCount();
	VkBuffer getVertexBuffer();
	void	 destroyVertexBuffer();

private:
	int				 m_vertexCount;
	VkBuffer		 m_vertexBuffer;
	VkDeviceMemory	 m_vertexBufferMemory;

	int				m_indexCount;
	VkBuffer		m_indexBuffer;
	VkDeviceMemory  m_indexBufferMemory;

	VkPhysicalDevice m_physicalDevice;
	VkDevice		 m_logicalDevice;

	void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
	void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
};

