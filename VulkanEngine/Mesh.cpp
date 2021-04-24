#include "Mesh.h"

// Crea la mesh partendo da "Vertex Data" e "Index Data"
Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, 
		   VkDevice newLogicalDevice, 
		   VkQueue transferQueue,
		   VkCommandPool transferCommandPool,
		   std::vector<Vertex>* vertices,
		   std::vector<uint32_t>* indices)
{
	m_vertexCount    = vertices->size();
	m_indexCount	 = indices->size();
	m_physicalDevice = newPhysicalDevice;
	m_logicalDevice  = newLogicalDevice;

	createVertexBuffer(transferQueue, transferCommandPool, vertices);
	createIndexBuffer(transferQueue, transferCommandPool, indices);

	m_model.model	 = glm::mat4(1.0f);
}

// Creazione del VertexBuffer
void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	// Dimesione in byte del Vertex Buffer
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// Buffer temporaneo tra il buffer della CPU ed il buffer GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Creazione dello Staging Buffer, la cui memoria è accedibile attraverso la CPU (buffer più lento)
	// e verranno bypassate tutte le operazioni di caching standard.
	// Il tipo del buffer è VK_BUFFER_USAGE_TRANSFER_SRC_BIT, significa che serve per effettuare un traferimento
	// dei dati all'interno del buffer verso un altra locazione.
	createBuffer(m_physicalDevice,
		m_logicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	//  Mapping della memoria sul Vertex Buffer
	void* data;																	 // 1. Creazione di un puntatore ad una locazione della memoria normale
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);  // 2. mapping tra la memoria del Vertex Buffer ed il pointer lato host
	memcpy(data, vertices->data(), static_cast<size_t>(bufferSize));			 // 3. Copio i Vertex Data nel buffer della GPU
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);						 // 4. Disassocio il vertice dalla memoria

	// Creazione di un buffer accessibile solo dalla GPU (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).
	// Il buffer è sia un buffer VK_BUFFER_USAGE_TRANSFER_DST_BIT
	// ovvero che è creato per ricevere dei dati da un altra locazione di memoria (lo staging buffer), ma è anche
	// VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ovvero un buffer per i Vertex Data.
	createBuffer(m_physicalDevice,
				 m_logicalDevice,
				 bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				 &m_vertexBuffer,
				 &m_vertexBufferMemory);

	// Copia lo staging buffer nel vertex buffer della GPU, è un operazione che viene effettuata attraverso
	// i CommandBuffer. Ovvero che è veloce perchè viene eseguita dalla GPU.
	copyBuffer(m_logicalDevice, transferQueue, transferCommandPool, stagingBuffer, m_vertexBuffer, bufferSize);

	// Distruzione dello StagingBuffer e liberazione della sua memoria
	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	// Prende la dimensione del buffer necessaria per gli indici
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// Buffer temporaneo per lo stae dei vertex data prima del trasferimento nella GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Creazione dello Staging Buffer ed allocazione della memoria in esso
	// Buffer per dati che devono essere trasferiti da qualche altra parte+
	createBuffer(m_physicalDevice, m_logicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	// Mapping della memoria per l'index buffer
	void* data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data); // 2. Creao le associazioni (mapping) tra la memoria del vertex buffer ed il pointer
	memcpy(data, indices->data(), static_cast<size_t>(bufferSize));			 // 3. Copio il vettore dei vertici in un punto in memoria
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);						 // 4. Disassocio il vertice dalla memoria

	// Creazione del buffer per Index Data sulla GPU
	createBuffer(m_physicalDevice, m_logicalDevice, 
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&m_indexBuffer, &m_indexBufferMemory);

	// Copia dello staging buffer sulla GPU
	copyBuffer(m_logicalDevice, transferQueue, transferCommandPool, stagingBuffer, m_indexBuffer, bufferSize);

	// Distruzione dello staging Buffer
	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
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
	vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_indexBufferMemory, nullptr);
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