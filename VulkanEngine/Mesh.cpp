#include "pch.h"

#include "Mesh.h"

Mesh::Mesh(MainDevice &mainDevice,
		   VkQueue transferQueue,
		   VkCommandPool transferCommandPool,
		   std::vector<Vertex>* vertices,
		   std::vector<uint32_t>* indices,
			int newTexID)
{
	m_vertexCount    = static_cast<int>(vertices->size());
	m_indexCount	 = static_cast<int>(indices->size());
	m_MainDevice	 = mainDevice;

	createVertexBuffer(transferQueue, transferCommandPool, vertices);
	createIndexBuffer(transferQueue, transferCommandPool, indices);

	m_model.model	 = glm::mat4(1.0f);
	m_texID = newTexID;
}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	// Creazione dello Staging Buffer, la cui memoria è accedibile attraverso la CPU (buffer più lento)
	// e verranno bypassate tutte le operazioni di caching standard.
	// Il tipo del buffer è VK_BUFFER_USAGE_TRANSFER_SRC_BIT, significa che serve per effettuare un traferimento
	// dei dati all'interno del buffer verso un altra locazione.
	Utility::CreateBuffer(m_MainDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&staging_buffer, &staging_buffer_memory);

	//  Mapping della memoria sul Vertex Buffer
	void* data;																	 // 1. Creazione di un puntatore ad una locazione della memoria normale
	vkMapMemory(m_MainDevice.LogicalDevice, staging_buffer_memory, 0, bufferSize, 0, &data);  // 2. mapping tra la memoria del Vertex Buffer ed il pointer lato host
	memcpy(data, vertices->data(), static_cast<size_t>(bufferSize));			 // 3. Copio i Vertex Data nel buffer della GPU
	vkUnmapMemory(m_MainDevice.LogicalDevice, staging_buffer_memory);						 // 4. Disassocio il vertice dalla memoria

	// Creazione di un buffer accessibile solo dalla GPU (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).
	// Il buffer è sia un buffer VK_BUFFER_USAGE_TRANSFER_DST_BIT
	// ovvero che è creato per ricevere dei dati da un altra locazione di memoria (lo staging buffer), ma è anche
	// VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ovvero un buffer per i Vertex Data.
	Utility::CreateBuffer(m_MainDevice,
				 bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				 &m_vertexBuffer,
				 &m_vertexBufferMemory);

	// Copia lo staging buffer nel vertex buffer della GPU, è un operazione che viene effettuata attraverso
	// i CommandBuffer. Ovvero che è veloce perchè viene eseguita dalla GPU.
	Utility::CopyBuffer(m_MainDevice.LogicalDevice, transferQueue, transferCommandPool, staging_buffer, m_vertexBuffer, bufferSize);
	
	vkDestroyBuffer(m_MainDevice.LogicalDevice, staging_buffer, nullptr);
	vkFreeMemory(m_MainDevice.LogicalDevice, staging_buffer_memory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	VkBuffer staging_buffer;
	VkDeviceMemory stagingBufferMemory;

	Utility::CreateBuffer(m_MainDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&staging_buffer, &stagingBufferMemory);

	// Mapping della memoria per l'index buffer
	void* data;
	vkMapMemory(m_MainDevice.LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data); // 2. Creao le associazioni (mapping) tra la memoria del vertex buffer ed il pointer
	memcpy(data, indices->data(), static_cast<size_t>(bufferSize));			 // 3. Copio il vettore dei vertici in un punto in memoria
	vkUnmapMemory(m_MainDevice.LogicalDevice, stagingBufferMemory);						 // 4. Disassocio il vertice dalla memoria

	// Creazione del buffer per Index Data sulla GPU
	Utility::CreateBuffer(m_MainDevice,
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&m_indexBuffer, &m_indexBufferMemory);

	// Copia dello staging buffer sulla GPU
	Utility::CopyBuffer(m_MainDevice.LogicalDevice, transferQueue, transferCommandPool, staging_buffer, m_indexBuffer, bufferSize);

	// Distruzione dello staging Buffer
	vkDestroyBuffer(m_MainDevice.LogicalDevice, staging_buffer, nullptr);
	vkFreeMemory(m_MainDevice.LogicalDevice, stagingBufferMemory, nullptr);
}

int Mesh::getVertexCount()
{
	return m_vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return m_vertexBuffer;
}

void Mesh::destroyBuffers()
{
	vkDestroyBuffer(m_MainDevice.LogicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_MainDevice.LogicalDevice, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_MainDevice.LogicalDevice, m_indexBuffer, nullptr);
	vkFreeMemory(m_MainDevice.LogicalDevice, m_indexBufferMemory, nullptr);
}

int Mesh::getIndexCount()
{
	return m_indexCount;
}

VkBuffer Mesh::getIndexBuffer()
{
	return m_indexBuffer;
}

void Mesh::setModel(glm::mat4 newModel)
{
	m_model.model = newModel;
}

Model Mesh::getModel()
{
	return m_model;
}

int Mesh::getTexID() const
{
	return m_texID;
}