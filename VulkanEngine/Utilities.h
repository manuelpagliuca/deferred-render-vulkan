#pragma once

#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 3;

const std::vector<const char*> deviceExtensions = 
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME			// SwapChain
};

// Rappresentazione dei vertici
struct Vertex
{
	glm::vec3 pos; // Posizione del vertice (x, y, z)
	glm::vec3 col; // Colore dei vertici (r,g,b)
};

// Indici contententi le posizioni delle Queue Families nel device fisico.
struct QueueFamilyIndices {

	// Famiglia che riguarda la grafica
	int graphicsFamily	   = -1;		// Location of Graphics Family Queue
	int presentationFamily = -1;		// Posizione della Presenation Queue Family (stesso tipo delle GraphicsFamily)
									
	bool isValid()	// Controlla se gli indici delle Queue Families sono validi
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

// Informazioni della surface che la swapchain può supportare
struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;	 // Proprietà della Surface, e.g. dimensioni immagine / estensioni
	std::vector<VkSurfaceFormatKHR> formats;		 // Lista dei formati immagini supportati dalla Surface (RGBA, ...)
	std::vector<VkPresentModeKHR> presentationModes; // Surface presentation modes (how the image is presented to screen)
};

struct SwapChainImage {
	VkImage image;			// Immagine fisica che verrà mostrata sul device
	VkImageView imageView;	// Interfaccia logica 
};

static std::vector<char> readFile(const std::string& filename)
{
	// Apre lo stream dal file fornito, come binario e partendo dal fondo (cosi' da poter calcolarne la dimensione)
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	// Controlliamo se il filestream e' stato aperto correttamente
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open a file!");
	}

	// Costruisce il fileBuffer con la dimensione del file SPIR-V
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> fileBuffer(fileSize);

	// Imposto il cursore del file all'inizio del file
	file.seekg(0);

	// Legge il contenuto del file e lo salva sul fileBuffer (ne legge ;fileSize' byte)
	file.read(fileBuffer.data(), fileSize);

	return fileBuffer;
}

// 
static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	// Prelevare le properietà della memoria del dispositivo fisico
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((allowedTypes & (1 << i)) && // Indice del tipo di memoria deve matchare con il bit corrispondente nel tipi permessi
			(memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			// Allora questo tipo di memoria è valido, quindi si restituisce il suo indice
			return i;
		}
	}

}

// Crea un buffer
static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
						 VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
						 VkMemoryAllocateFlags bufferProperties,
						 VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	/* Creazione del Vertex Buffer */
	// Informazioni per la creazione del buffer (non comporta l'assegnamento della memoria)
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; // Struttura
	bufferInfo.size  = bufferSize;	   // Dimensione del buffer
	bufferInfo.usage = bufferUsage;	   // VertexBuffer
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			   // Simile alla SwapChain Images, può condividere vertex buffers

	VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vertex Buffer!");
	}

	// Prelevare i requisiti di memoria del buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements); // Preleva i requisiti

	// Allocare la memoria del buffer
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;					// Quanto vogliamo allocare per il nostro buffer
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice,
														  memRequirements.memoryTypeBits,
														  bufferProperties); // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU può interagire con la memoria
																			 // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Permette il posizionamento dei dati direttamente nel buffer dopo il mapping
																			 // (altrimenti ha bisogno di essere specificato manualmente)
	// Allocazione della memoria in VkDeviceMemory
	result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
	}

	// Allocazione della memoria sul dato buffer
	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

static void copyBuffer(VkDevice	logicalDevice, VkQueue transferQueue,
					   VkCommandPool transferCommandPool, VkBuffer srcBuffer,
					   VkBuffer	dstBuffer, VkDeviceSize bufferSize)
{
	// Command buffer che deve contenere i comandi
	VkCommandBuffer transferCommandBuffer;

	// Dettagli del Command Buffer
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = transferCommandPool;
	allocInfo.commandBufferCount = 1;

	// Allocare il commandbuffer dalla pool
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &transferCommandBuffer);

	// Informazioni per iniziare la registrazione del command buffer
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Inizia la registrazione del comando di trasferimento
	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

	// Regione dei dati da cui voglio copiare
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	// Comando per copiare il srcBuffer nel dstBuffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// Termina i comandi
	vkEndCommandBuffer(transferCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType			  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &transferCommandBuffer;

	// Invia i comando di trasferimento alla transfer queue ed aspetta finchè termini
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	// Libera i command buffer temporanei
	vkFreeCommandBuffers(logicalDevice, transferCommandPool, 1, &transferCommandBuffer);
}