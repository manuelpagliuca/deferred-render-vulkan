#include "Utilities.h"

// Legge un file
std::vector<char> readFile(const std::string& filename)
{
	// Apre lo stream dal file fornito, come binario e partendo dal fondo
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open a file!");
	}

	size_t fileSize = static_cast<size_t>(file.tellg()); // Posizione del cursore = Dimensione File
	file.seekg(0);										 // Riposiziono il cursore all'inizio 
	std::vector<char> fileBuffer(fileSize);				 // Costruzione del file buffer
	file.read(fileBuffer.data(), fileSize);				 // Legge il contenuto dello stream e lo salva sul buffer

	return fileBuffer;
}

// Crea un buffer sulla GPU
void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
	VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
	VkMemoryAllocateFlags bufferProperties,
	VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size		   = bufferSize;				// Dimensione del buffer
	bufferInfo.usage	   = bufferUsage;				// Utilizzo del buffer (VertexBuffer, IndexBuffer, UniformBuffer, ...)
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// Accesso alle immagini esclusivo alla Queue Family

	VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vertex Buffer!");
	}

	// Query per i requisiti di memoria del buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

	// Informazioni per l'allocazione della memoria del buffer
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType		    = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize  = memRequirements.size;					// Dimensione dell'allocazione = Memoria richiesta per il buffer
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(					// Indice del tipo di memoria da utilizzare
										physicalDevice,
										memRequirements.memoryTypeBits, 
										bufferProperties); 

	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU può interagire con la memoria
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Permette il posizionamento dei dati direttamente nel buffer dopo il mapping
	// (altrimenti ha bisogno di essere specificato manualmente)

	result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
	}

	// Allocazione della memoria sul dato buffer
	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

// Restituisce l'indice per il tipo di memoria desiderato, a partire dal tipo definito dal requisito di memoria
uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t supportedMemoryTypes, VkMemoryPropertyFlags properties)
{
	// Query per le proprietà della memoria
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	// Per ogni tipo di memoria 
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		auto firstCond = supportedMemoryTypes & (1 << i);
		auto secondCond = memoryProperties.memoryTypes[i].propertyFlags & properties;// Indice del tipo di memoria deve matchare con il bit corrispondente nel tipi permessi

		if (firstCond && (secondCond == properties))
		{
			// Allora questo tipo di memoria è valido, quindi si restituisce il suo indice
			return i;
		}
	}
}

// Copia dei dati di un buffer in un altro attraverso dei CommandBuffer (quindi l'operazione è eseguita dalla GPU)
void copyBuffer(VkDevice logicalDevice, VkQueue transferQueue,
				VkCommandPool transferCommandPool, VkBuffer srcBuffer,
				VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	VkCommandBuffer transferCommandBuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType				 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY; 
	allocInfo.commandPool		 = transferCommandPool;				
	allocInfo.commandBufferCount = 1;								

	// Alloca il CommandBuffer dalla pool
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &transferCommandBuffer);

	// Informazioni per iniziare la registrazione del Command Buffer
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Ogni comando registrato nel Command Buffer verrà inviato soltanto una volta

	// Inizia la registrazione del comando di trasferimento
	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

		// Regione dei dati sorgente (da cui voglio copiare)
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = 0;
		bufferCopyRegion.dstOffset = 0;
		bufferCopyRegion.size	   = bufferSize;

		// Comando per copiare il srcBuffer nel dstBuffer
		vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// Termina la registrazione del comando nel CommandBuffer
	vkEndCommandBuffer(transferCommandBuffer);

	// Informazioni per effettuare la submit del CommandBuffer alla Queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType			  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &transferCommandBuffer;

	// Invia li comando di copia alla Transfer Queue (nel nostro caso sarà la Graphics Queue) ed aspetta finchè termini
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	// Libera il CommandBuffer
	vkFreeCommandBuffers(logicalDevice, transferCommandPool, 1, &transferCommandBuffer);
}