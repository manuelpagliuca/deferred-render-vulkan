#include "Utilities.h"


#include "stb_image.h"
#define STB_IMAGE_IMPLEMENTATION


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
// supportedMemoryTypes : Requisiti di memoria del buffer
// properties			: Proprietà della memoria (come la visibilità/accesso del buffer)
uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t supportedMemoryTypes, VkMemoryPropertyFlags properties)
{
	// Query per le proprietà della memoria
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	// Per ogni tipo di memoria 
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		// Itera per ogni tipo di memoria shiftando a sinistra un 1
		// in questo modo crea delle varie combinazioni di tipologie di memoria.
		// Se esiste una combinazione di memoria che è supportata e che quindi è contenuta in "supportedMemoryTypes"
		// questo significa che si prenderà il tipo di memoria che supporta tutti 
		// i flag possibili (tutti i bit saranno ad 1, quindi con valore 256).
		auto supportedMemory  = supportedMemoryTypes & (1U << i);

		// Itera per ogni tipo di memoria e fa AND BIT-a-BIT con le proprietà del tipo di memoria.
		// Se dopo questa operazione, il risultato è uguale all proprietà della memoria fornite.
		// Allora quella proprietà è supportata dall'i-esimo indice
		auto supportedProperties = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;

		// Se il tipo di memoria è stato trovato assieme alle proprietà durante lo stesso ciclo
		// significa che il tipo di memoria è valido e ne restituiamo l'indice.
		if (supportedMemory && supportedProperties)
		{
			return i;
		}
	}
}

VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	// Alloca il CommandBuffer dalla pool
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	// Informazioni per iniziare la registrazione del Command Buffer
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Ogni comando registrato nel Command Buffer verrà inviato soltanto una volta

	// Inizia la registrazione del comando di trasferimento
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
	// Termina la registrazione del comando nel CommandBuffer
	vkEndCommandBuffer(commandBuffer);

	// Informazioni per effettuare la submit del CommandBuffer alla Queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Invia li comando di copia alla Transfer Queue (nel nostro caso sarà la Graphics Queue) ed aspetta finchè termini
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	// Libera il CommandBuffer
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// Copia dei dati di un buffer in un altro attraverso dei CommandBuffer (quindi l'operazione è eseguita dalla GPU)
void copyBuffer(VkDevice device, VkQueue transferQueue,
				VkCommandPool transferCommandPool, VkBuffer srcBuffer,
				VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

	// Regione dei dati sorgente (da cui voglio copiare)
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size	   = bufferSize;

	// Comando per copiare il srcBuffer nel dstBuffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer src, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

	VkBufferImageCopy imageRegion = {};
	imageRegion.bufferOffset					= 0;
	imageRegion.bufferRowLength					= 0;
	imageRegion.bufferImageHeight				= 0;
	imageRegion.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	imageRegion.imageSubresource.mipLevel		= 0;
	imageRegion.imageSubresource.baseArrayLayer = 0;
	imageRegion.imageSubresource.layerCount		= 1;
	imageRegion.imageOffset = { 0, 0, 0 };
	imageRegion.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(transferCommandBuffer, src, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

	endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType							  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout						  = oldLayout;					// Layout da cui spostarsi
	imageMemoryBarrier.newLayout						  = newLayout;					// Layout in cui spostarsi
	imageMemoryBarrier.srcQueueFamilyIndex			  = VK_QUEUE_FAMILY_IGNORED;	// Queue family da cui spostarsi
	imageMemoryBarrier.dstQueueFamilyIndex			  = VK_QUEUE_FAMILY_IGNORED;	// Queue family in cui spostarsi
	imageMemoryBarrier.image							  = image;						// Immagine che viene acceduta e modificata come parte della barriera
	imageMemoryBarrier.subresourceRange.aspectMask	  = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.baseMipLevel	  = 0;
	imageMemoryBarrier.subresourceRange.levelCount	  = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount	  = 1;

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;


	
	// Se si sta transizionando da una nuova immagine ad un immagine pronta per ricevere i dati...
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = 0;							// Qualsiasi stage iniziale
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	// Questo è lo stage che viene eseguito nel momento che lo stage nel 'srcAccessMask' viene completato

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;	// qualsiasi momento dopo l'inizio della pipeline
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;		// Primache provi a fare una write nel transfer stage!
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;			// al termine delle operazioni di scrittura del transfer stage
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;	// prima che provi a a leggere il fragment shader
	}

	vkCmdPipelineBarrier(commandBuffer,
		srcStage, dstStage,  // Pipeline stages sono connessi ai memory access (srcAccessMAsk ...)
		0,	   // Dependencey flag
		0, nullptr,	// Memory Barrier count + data
		0, nullptr,	// Buffer memory barrier + data
		1, &imageMemoryBarrier// ImageMemoryBarrier  count + data
	);

	endAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}
