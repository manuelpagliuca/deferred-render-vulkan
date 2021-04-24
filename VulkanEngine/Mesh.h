#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

struct Model {
	glm::mat4 model;
};

class Mesh
{
public:
	Mesh()  = default;
	~Mesh() = default;
	Mesh(VkPhysicalDevice newPhysicalDevice,
		 VkDevice newLogicalDevice, 
		 VkQueue transferQueue, 
		 VkCommandPool transferCommandPool, 
		 std::vector<Vertex>* vertices,
		 std::vector<uint32_t>* indices);

	int		 getVertexCount();
	VkBuffer getVertexBuffer();
	void	 destroyBuffers();

	int		 getIndexCount();
	VkBuffer getIndexBuffer();

	void setModel(glm::mat4 newModel);
	Model getModel();
	const void* getData() { return &m_model; }

private:
	Model m_model;

	/* Vertex Data */
	int				 m_vertexCount;
	VkBuffer		 m_vertexBuffer;
	VkDeviceMemory	 m_vertexBufferMemory;

	/* Index Data */
	int				 m_indexCount;
	VkBuffer		 m_indexBuffer;
	VkDeviceMemory   m_indexBufferMemory;

	/* Devices */
	VkPhysicalDevice m_physicalDevice;
	VkDevice		 m_logicalDevice;

private:
	void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
	void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
};

