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
	Mesh(MainDevice &m_MainDevice,
		 VkQueue transferQueue, 
		 VkCommandPool transferCommandPool, 
		 std::vector<Vertex>* vertices,
		 std::vector<uint32_t>* indices,
		 int newTexID);

	int		 getVertexCount();
	VkBuffer getVertexBuffer();
	void	 destroyBuffers();

	int		 getIndexCount();
	VkBuffer getIndexBuffer();

	int		 getTexID() const;

	void setModel(glm::mat4 newModel);
	Model getModel();
	const void* getData() { return &m_model; }

private:
	MainDevice		 m_MainDevice;
	Model m_model;
	int m_texID;

	/* Vertex Data */
	int				 m_vertexCount;
	VkBuffer		 m_vertexBuffer;
	VkDeviceMemory	 m_vertexBufferMemory;

	/* Index Data */
	int				 m_indexCount;
	VkBuffer		 m_indexBuffer;
	VkDeviceMemory   m_indexBufferMemory;

private:
	void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
	void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
};

