#include "pch.h"

#include "Utilities.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

std::vector<char> Utility::ReadFile(const std::string& filename)
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

void Utility::CreateBuffer(MainDevice &mainDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
	VkMemoryAllocateFlags bufferProperties,	VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size		   = bufferSize;				// Dimensione del buffer
	bufferInfo.usage	   = bufferUsage;				// Utilizzo del buffer (VertexBuffer, IndexBuffer, UniformBuffer, ...)
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// Accesso alle immagini esclusivo alla Queue Family

	VkResult result = vkCreateBuffer(mainDevice.LogicalDevice, &bufferInfo, nullptr, buffer);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vertex Buffer!");
	}

	// Query per i requisiti di memoria del buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mainDevice.LogicalDevice, *buffer, &memRequirements);

	// Informazioni per l'allocazione della memoria del buffer
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType		    = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize  = memRequirements.size;					// Dimensione dell'allocazione = Memoria richiesta per il buffer
	memoryAllocInfo.memoryTypeIndex = Utility::FindMemoryTypeIndex(mainDevice.PhysicalDevice, memRequirements.memoryTypeBits, bufferProperties);

	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU può interagire con la memoria
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Permette il posizionamento dei dati direttamente nel buffer dopo il mapping
	// (altrimenti ha bisogno di essere specificato manualmente)

	result = vkAllocateMemory(mainDevice.LogicalDevice, &memoryAllocInfo, nullptr, bufferMemory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
	}

	// Allocazione della memoria sul dato buffer
	vkBindBufferMemory(mainDevice.LogicalDevice, *buffer, *bufferMemory, 0);
}

// Restituisce l'indice per il tipo di memoria desiderato, a partire dal tipo definito dal requisito di memoria
// supportedMemoryTypes : Requisiti di memoria del buffer
// properties			: Proprietà della memoria (come la visibilità/accesso del buffer)

VkCommandBuffer Utility::BeginCommandBuffer(VkDevice device, VkCommandPool commandPool)
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

void Utility::EndAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
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
void Utility::CopyBuffer(VkDevice& logicalDevice, VkQueue& transferQueue,
	VkCommandPool& transferCommandPool, VkBuffer& srcBuffer,
	VkBuffer& dstBuffer, VkDeviceSize bufferSize)
{
	VkCommandBuffer transferCommandBuffer = BeginCommandBuffer(logicalDevice, transferCommandPool);

	// Regione dei dati sorgente (da cui voglio copiare)
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size	   = bufferSize;

	// Comando per copiare il srcBuffer nel dstBuffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	EndAndSubmitCommandBuffer(logicalDevice, transferCommandPool, transferQueue, transferCommandBuffer);
}

void Utility::CopyImageBuffer(VkDevice &device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer src, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer transferCommandBuffer = BeginCommandBuffer(device, transferCommandPool);

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

	EndAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}
/*
Model* Utility::AllocateDynamicBufferTransferSpace(VkDeviceSize minUniformBufferOffset)
{
	// Allineamento dei dati per la Model
	/*
	Si vuole scoprire l'allineamento da utilizzare per la Model Uniform. Si utilizzano varie operazioni di bitwise.
	e.g, fingiamo che la dimensione minima di offset sia di 32-bit quindi rappresentata da 00100000.
	Al di sopra di questa dimensione di bit sono accettati tutti i suoi multipli (32,64,128,...).

	Quindi singifica che se la dimensione della Model è di 16-bit non sarà presente un allineamento per la memoria
	che sia valido, mentre 128-bit andrà bene. Per trovare questo allineamento si utilizza una maschera di per i 32-bit
	in modo da poterla utilizzare con l'operazione BITWISE di AND (&) con la dimensione effettiva in byte della Model.

	Il risultato di questa operazioni saranno i byte dell'allineamento necessari.
	Per la creazione della maschera stessa si utilizza il complemento a 1 seguito dall'inversione not (complemento a 1).

	32-bit iniziali (da cui voglio creare la maschera) : 00100000
	32-bit meno 1 (complemento a 2) : 00011111
	32-bit complemento a 1 : 11100000 (maschera di bit risultante)

	maschera : ~(minOffset -1)
	Allineamento risultante : sizeof(UboModel) & maschera

	Tutto funziona corretto finchè non si vuole utilizzare una dimensione che non è un multiplo diretto della maschera
	per esempio 00100010, in questo caso la mascherà risultante sarà 11100000 ma questa da un allineamento di 00100000.
	cosa che non è corretta perchè si perde il valore contenuto nella penultima cifra!

	Come soluzione a questa problematica si vuole spostare la cifra che verrebbe persa subito dopo quella risultante
	dall'allineamento, in questo modo l'allineamento dovrebbe essere corretto.

	Per fare ciò aggiungiamo 32 bit e facciamo il complemento a 2 su questi 32-bit.
	00100000 => 00011111
	Ora aggiungendo questi bit alla dimensione della Model dovrebbe essere protetta.
	*//*
	unsigned long long const offsetMask			= ~(minUniformBufferOffset - 1);
	unsigned long long const protectedModelSize = (sizeof(Model) + minUniformBufferOffset - 1);
	size_t modelUniformAlignment = protectedModelSize & offsetMask;
	
	// Allocazione spazio in memoria per gestire il dynamic buffer che è allineato, e con un limite di oggetti
	return static_cast<Model*>(_aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment));
}

void Utility::FreeAlignedMemoryDUBO(Model * modelTransferSpace)
{
	_aligned_free(modelTransferSpace);
}
*/
void Utility::TransitionImageLayout(VkDevice &device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginCommandBuffer(device, commandPool);

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

	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;
	
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

	EndAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}

VkImage Utility::CreateImage(MainDevice &mainDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
{
	// CREAZIONE DELL'IMMAGINE (Header dell'immagine, in memoria non è ancora presente)
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;			// NO 3D ASPECT
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout dell'immagine al momento della creazione
	imageCreateInfo.usage = useFlags;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkImage image;
	VkResult result = vkCreateImage(mainDevice.LogicalDevice, &imageCreateInfo, nullptr, &image);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image!");
	}

	// CREAZIONE DELLA MEMORIA PER L'IMMAGINE
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(mainDevice.LogicalDevice, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(mainDevice.PhysicalDevice, memoryRequirements.memoryTypeBits, propFlags);

	result = vkAllocateMemory(mainDevice.LogicalDevice, &memoryAllocInfo, nullptr, imageMemory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate memory for image!");
	}

	result = vkBindImageMemory(mainDevice.LogicalDevice, image, *imageMemory, 0);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to bind memory to the image!");
	}

	return image;
}

VkImageView Utility::CreateImageView(VkDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspetFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;									// Immagine pre-esistente nella SwapChain
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;					// Tipo delle immagini (2D)
	viewCreateInfo.format = format;									// Formato delle immagini (stesso della Surface)
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;			// Imposto lo swizzle con il valore della componente effettiva
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// L'intervallo delle sottorisorse dicono all'ImageView quale parte dell'immagine visualizzare
	viewCreateInfo.subresourceRange.aspectMask = aspetFlags; // Quale aspetto dell'immagine visualizzare (COLOR, DEPTH, STENCIL)
	viewCreateInfo.subresourceRange.baseMipLevel = 0;			 // Livello iniziale della mipmap (primo mipmap level)
	viewCreateInfo.subresourceRange.levelCount = 1;			 // Numero di livelli mipmap da visualizzare
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;			 // Livello iniziale del primo arrayLayer (primo arrayLayer)
	viewCreateInfo.subresourceRange.layerCount = 1;			 // Numero di ArrayLayer

	VkImageView imageView;
	VkResult res = vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image View!");
	}

	return imageView;
}

void Utility::CreateDepthBufferImage(DepthBufferImage& image, MainDevice &mainDevice, VkExtent2D imgExtent)
{
	// Depth value of 32bit expressed in floats + stencil buffer (non lo usiamo ma è utile averlo disponibile)
// Nel caso in cui lo stencil buffer non sia disponibile andiamo a prendere solo il depth buffer 32 bit.
// Se non è disponibile neanche quello da 32 bit si prova con quello di 24 (poi non proviamo più).
	image.Format = Utility::ChooseSupportedFormat(mainDevice, { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	image.Image = Utility::CreateImage(mainDevice, imgExtent.width, imgExtent.height,
		image.Format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &image.Memory);

	image.ImageView = Utility::CreateImageView(mainDevice.LogicalDevice, image.Image, image.Format, VK_IMAGE_ASPECT_DEPTH_BIT);


}

uint32_t Utility::FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t supportedMemoryTypes, VkMemoryPropertyFlags properties)
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
		auto supportedMemory = supportedMemoryTypes & (1U << i);

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

	return static_cast<uint32_t>(0);
}

VkShaderModule Utility::CreateShaderModule(VkDevice &device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();									  // Dimensione del codice SPIR-V
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // Puntatore al codice SPIR-V

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Shader Module!");
	}

	return shaderModule;
}

int Utility::CreateTexture(
	MainDevice &mainDevice , VkDescriptorPool &texturePool, VkDescriptorSetLayout &textureLayout,
	TextureObjects & textureObjects, VkQueue& graphicsQueue, 
	VkCommandPool& graphicsCommandPool,
	std::string fileName)
{
	int textureImageLoc = CreateTextureImage(mainDevice, textureObjects, graphicsQueue, graphicsCommandPool, fileName);

	VkImageView imageView = Utility::CreateImageView(mainDevice.LogicalDevice, textureObjects.TextureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	textureObjects.TextureImageViews.push_back(imageView);

	// TODO : Create Descriptor Set Here
	int descriptorLoc = CreateTextureDescriptor(mainDevice.LogicalDevice, imageView, texturePool, textureLayout, textureObjects);

	return descriptorLoc;
}

int Utility::CreateTextureImage(MainDevice& mainDevice, TextureObjects &textureObjects, 
	VkQueue &graphicsQueue, VkCommandPool &graphicsCommandPool, std::string fileName)
{
	int width, height;
	VkDeviceSize imageSize;
	stbi_uc* imageData = LoadTextureFile(fileName, &width, &height, &imageSize);

	// staging buffer
	VkBuffer imageStaginBuffer;
	VkDeviceMemory imageStagingBufferMemory;
	CreateBuffer(mainDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStaginBuffer, &imageStagingBufferMemory);

	// copy image data to staging buffer
	void* data;
	vkMapMemory(mainDevice.LogicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, imageData, static_cast<size_t>(imageSize));
	vkUnmapMemory(mainDevice.LogicalDevice, imageStagingBufferMemory);

	stbi_image_free(imageData);

	// create image to hold final texture
	VkImage texImage;
	VkDeviceMemory texImageMemory;
	texImage = Utility::CreateImage(
		mainDevice,
		width, height,
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

	// Descrive la pipeline barrier che deve rispettare la queue
	TransitionImageLayout(mainDevice.LogicalDevice, graphicsQueue, graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyImageBuffer(mainDevice.LogicalDevice, graphicsQueue, graphicsCommandPool, imageStaginBuffer, texImage, width, height);
	TransitionImageLayout(mainDevice.LogicalDevice, graphicsQueue, graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	textureObjects.TextureImages.push_back(texImage);
	textureObjects.TextureImageMemory.push_back(texImageMemory);

	// Destroying staging buffer
	vkDestroyBuffer(mainDevice.LogicalDevice, imageStaginBuffer, nullptr);
	vkFreeMemory(mainDevice.LogicalDevice, imageStagingBufferMemory, nullptr);

	// Restituisce l'indice della nuova texture image
	return static_cast<int>(textureObjects.TextureImages.size()) - 1;
}

stbi_uc* Utility::LoadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize)
{
	int nChannels;

	std::string fileLoc = "Textures/" + fileName;
	stbi_uc* image = stbi_load(fileLoc.c_str(), width, height, &nChannels, STBI_rgb_alpha);

	if (!image)
	{
		throw std::runtime_error("Failed to load a Texture file! (" + fileName + ")");
	}

	*imageSize = static_cast<uint64_t>((*width)) * static_cast<uint64_t>((*height)) * 4L;

	return image;

	return nullptr;
}

int Utility::CreateTextureDescriptor(VkDevice &device, VkImageView textureImage, VkDescriptorPool &texturePool, VkDescriptorSetLayout &textureLayout, TextureObjects &textureObjects)
{
	VkDescriptorSet descriptorSet;

	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = texturePool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &textureLayout;

	VkResult result = vkAllocateDescriptorSets(device, &setAllocInfo, &descriptorSet);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Texture Descriptor Set!");
	}

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImage;
	imageInfo.sampler = textureObjects.TextureSampler;


	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

	textureObjects.SamplerDescriptorSets.push_back(descriptorSet);

	return static_cast<int>(textureObjects.SamplerDescriptorSets.size()) - 1;
}

VkFormat Utility::ChooseSupportedFormat(MainDevice &mainDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	for (VkFormat format : formats)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(mainDevice.PhysicalDevice, format, &properties);

		// In base alle scelte di tiling si sceglie un bitflag differente
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
	}
	throw std::runtime_error("Failed to find a matching format!");
}

void Utility::GetQueueFamilyIndices(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface, QueueFamilyIndices& queueFamilyIndices)
{
	// Prelevo le Queue Families disponibili nel dispositivo fisico
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyList.data());

	// Indice delle Queue Families (a carico del developer)
	int queueIndex = 0;

	if (queueFamilyCount > 0)
	{
		for (const auto& queueFamily : queueFamilyList)
		{
			// Se la QueueFamily è valida ed è una Queue grafica, salvo l'indice nel Renderer
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queueFamilyIndices.GraphicsFamily = queueIndex;
			}

			VkBool32 presentationSupport = false;

			/*	Query per chiedere se è supportata la Presentation Mode nella Queue Family.
				Solitamente le Queue Family appartenenti alla grafica la supportano.
				Questa caratteristica è obbligatoria per presentare l'immagine dalla SwapChain alla Surface.*/
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, surface, &presentationSupport);

			// Se la Queue Family supporta la Presentation Mode, salvo l'indice nel Renderer (solitamente sono gli stessi indici)
			if (presentationSupport)
			{
				queueFamilyIndices.PresentationFamily = queueIndex;
			}

			if (queueFamilyIndices.isValid())
			{
				break;
			}

			++queueIndex;
		}
	}
}

bool Utility::CheckDeviceExtensionSupport(VkPhysicalDevice& device, const std::vector<const char*>& requestedDeviceExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0)
		return false;

	std::vector<VkExtensionProperties> physicalDeviceExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, physicalDeviceExtensions.data());

	for (const auto& extensionToCheck : requestedDeviceExtensions)
	{
		bool hasExtension = false;

		for (const auto& extension : physicalDeviceExtensions)
		{
			if (strcmp(extensionToCheck, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
			return false;
	}

	return true;
}
