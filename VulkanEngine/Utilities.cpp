#include "pch.h"

#include "Utilities.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

MainDevice*		Utility::m_MainDevice		= nullptr;
VkSurfaceKHR*	Utility::m_Surface			= nullptr;
VkCommandPool*	Utility::m_CommandPool		= nullptr;
VkQueue*		Utility::m_GraphicsQueue	= nullptr;

void Utility::Setup(MainDevice* main_device, VkSurfaceKHR* surface, VkCommandPool* command_pool, VkQueue* queue)
{
	m_MainDevice	= main_device;
	m_Surface		= surface;
	m_CommandPool	= command_pool;
	m_GraphicsQueue = queue;
}

std::vector<char> Utility::ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open a file!");
	}

	size_t file_size = static_cast<size_t>(file.tellg());	
	file.seekg(0);											
	std::vector<char> file_buffer(file_size);				
	file.read(file_buffer.data(), file_size);				

	return file_buffer;
}

void Utility::CreateBuffer(const BufferSettings& buffer_settings, VkBuffer* buffer_data, VkDeviceMemory* memory)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size			= buffer_settings.size;		
	buffer_info.usage			= buffer_settings.usage;	
	buffer_info.sharingMode		= VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(m_MainDevice->LogicalDevice, &buffer_info, nullptr, buffer_data);
	
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vertex Buffer!");
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(m_MainDevice->LogicalDevice, *buffer_data, &mem_requirements);

	VkMemoryAllocateInfo memory_alloc_info = {};
	memory_alloc_info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_alloc_info.allocationSize	= mem_requirements.size;					
	memory_alloc_info.memoryTypeIndex	= Utility::FindMemoryTypeIndex(mem_requirements.memoryTypeBits, buffer_settings.properties);

	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU può interagire con la memoria
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Permette il posizionamento dei dati direttamente nel buffer dopo il mapping
	// (altrimenti ha bisogno di essere specificato manualmente)

	result = vkAllocateMemory(m_MainDevice->LogicalDevice, &memory_alloc_info, nullptr, memory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
	}

	vkBindBufferMemory(m_MainDevice->LogicalDevice, *buffer_data, *memory, 0);
}

VkCommandBuffer Utility::BeginCommandBuffer()
{
	VkCommandBuffer command_buffer;

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool			= *m_CommandPool;
	alloc_info.commandBufferCount	= 1;

	vkAllocateCommandBuffers(m_MainDevice->LogicalDevice, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Ogni comando registrato nel Command Buffer verrà inviato soltanto una volta

	vkBeginCommandBuffer(command_buffer, &beginInfo);

	return command_buffer;
}

void Utility::EndAndSubmitCommandBuffer(const VkCommandBuffer &command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info		= {};
	submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount	= 1;
	submit_info.pCommandBuffers		= &command_buffer;

	// Invia li comando di copia alla Transfer Queue (nel nostro caso sarà la Graphics Queue) ed aspetta finchè termini
	vkQueueSubmit(*m_GraphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(*m_GraphicsQueue);

	vkFreeCommandBuffers(m_MainDevice->LogicalDevice, *m_CommandPool, 1, &command_buffer);
}

void Utility::CopyBufferCmd(const VkBuffer& src_buffer, const VkBuffer& dst_buffer, const VkDeviceSize& buffer_size)
{
	VkCommandBuffer transfer_command_buffer = BeginCommandBuffer();

	VkBufferCopy buffer_copy_region = {};
	buffer_copy_region.srcOffset = 0;
	buffer_copy_region.dstOffset = 0;
	buffer_copy_region.size		 = buffer_size;

	vkCmdCopyBuffer(transfer_command_buffer, src_buffer, dst_buffer, 1, &buffer_copy_region);

	EndAndSubmitCommandBuffer(transfer_command_buffer);
}

void Utility::CopyImageBuffer(const VkBuffer &src, const VkImage &image, const uint32_t width, const uint32_t height)
{
	VkCommandBuffer transfer_command_buffer = BeginCommandBuffer();

	VkBufferImageCopy image_region = {};
	image_region.bufferOffset						= 0;
	image_region.bufferRowLength					= 0;
	image_region.bufferImageHeight					= 0;
	image_region.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	image_region.imageSubresource.mipLevel			= 0;
	image_region.imageSubresource.baseArrayLayer	= 0;
	image_region.imageSubresource.layerCount		= 1;
	image_region.imageOffset						= { 0, 0, 0 };
	image_region.imageExtent						= { width, height, 1 };

	vkCmdCopyBufferToImage(transfer_command_buffer, src, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_region);

	EndAndSubmitCommandBuffer(transfer_command_buffer);
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


VkImage Utility::CreateImage(const ImageInfo& image_info, VkDeviceMemory* imageMemory)
{
	// CREAZIONE DELL'IMMAGINE (Header dell'immagine, in memoria non è ancora presente)
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType		= VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width	= image_info.width;
	imageCreateInfo.extent.height	= image_info.height;
	imageCreateInfo.extent.depth	= 1;			// NO 3D ASPECT
	imageCreateInfo.mipLevels		= 1;
	imageCreateInfo.arrayLayers		= 1;
	imageCreateInfo.format			= image_info.format;
	imageCreateInfo.tiling			= image_info.tiling;
	imageCreateInfo.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED; // layout dell'immagine al momento della creazione
	imageCreateInfo.usage			= image_info.usage;
	imageCreateInfo.samples			= VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode		= VK_SHARING_MODE_EXCLUSIVE;

	VkImage image;
	VkResult result = vkCreateImage(m_MainDevice->LogicalDevice, &imageCreateInfo, nullptr, &image);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image!");
	}

	// CREAZIONE DELLA MEMORIA PER L'IMMAGINE
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_MainDevice->LogicalDevice, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, image_info.properties);

	result = vkAllocateMemory(m_MainDevice->LogicalDevice, &memoryAllocInfo, nullptr, imageMemory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate memory for image!");
	}

	result = vkBindImageMemory(m_MainDevice->LogicalDevice, image, *imageMemory, 0);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to bind memory to the image!");
	}

	return image;
}

VkImageView Utility::CreateImageView(const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspect_flags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image		= image;									// Immagine pre-esistente nella SwapChain
	viewCreateInfo.viewType		= VK_IMAGE_VIEW_TYPE_2D;					// Tipo delle immagini (2D)
	viewCreateInfo.format		= format;									// Formato delle immagini (stesso della Surface)
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;			// Imposto lo swizzle con il valore della componente effettiva
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// subresourceRange : dice all'ImageView quale parte dell'image visualizzare
	viewCreateInfo.subresourceRange.aspectMask		= aspect_flags;	 // Quale aspetto dell'immagine visualizzare (COLOR, DEPTH, STENCIL)
	viewCreateInfo.subresourceRange.baseMipLevel	= 0;			 // Livello iniziale della mipmap (primo mipmap level)
	viewCreateInfo.subresourceRange.levelCount		= 1;			 // Numero di livelli mipmap da visualizzare
	viewCreateInfo.subresourceRange.baseArrayLayer	= 0;			 // Livello iniziale del primo arrayLayer (primo arrayLayer)
	viewCreateInfo.subresourceRange.layerCount		= 1;			 // Numero di ArrayLayer

	VkImageView imageView;
	VkResult res = vkCreateImageView(m_MainDevice->LogicalDevice, &viewCreateInfo, nullptr, &imageView);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image View!");
	}

	return imageView;
}

void Utility::CreateDepthBufferImage(DepthBufferImage& image, const VkExtent2D &image_extent)
{
	// Depth value of 32bit expressed in floats + stencil buffer (non lo usiamo ma è utile averlo disponibile)
// Nel caso in cui lo stencil buffer non sia disponibile andiamo a prendere solo il depth buffer 32 bit.
// Se non è disponibile neanche quello da 32 bit si prova con quello di 24 (poi non proviamo più).
	const std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
	const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	const VkFormatFeatureFlagBits format_flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

	image.Format = Utility::ChooseSupportedFormat(formats, tiling, format_flags);

	ImageInfo image_info = {};
	image_info.width		= image_extent.width;
	image_info.height		= image_extent.height;
	image_info.format		= image.Format;
	image_info.tiling		= tiling;
	image_info.usage		= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_info.properties	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	image.Image = Utility::CreateImage(image_info, &image.Memory);

	image.ImageView = Utility::CreateImageView(image.Image, image.Format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

uint32_t Utility::FindMemoryTypeIndex(uint32_t supportedMemoryTypes, const VkMemoryPropertyFlags &properties)
{
	// Query per le proprietà della memoria
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(m_MainDevice->PhysicalDevice, &memory_properties);

	// Per ogni tipo di memoria 
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
	{
		// Itera per ogni tipo di memoria shiftando a sinistra un 1
		// in questo modo crea delle varie combinazioni di tipologie di memoria.
		// Se esiste una combinazione di memoria che è supportata e che quindi è contenuta in "supportedMemoryTypes"
		// questo significa che si prenderà il tipo di memoria che supporta tutti 
		// i flag possibili (tutti i bit saranno ad 1, quindi con valore 256).
		unsigned int supported_memory = supportedMemoryTypes & (1U << i);

		// Itera per ogni tipo di memoria e fa AND BIT-a-BIT con le proprietà del tipo di memoria.
		// Se dopo questa operazione, il risultato è uguale all proprietà della memoria fornite.
		// Allora quella proprietà è supportata dall'i-esimo indice
		bool supported_properties = (memory_properties.memoryTypes[i].propertyFlags & properties) == properties;

		// Se il tipo di memoria è stato trovato assieme alle proprietà durante lo stesso ciclo
		// significa che il tipo di memoria è valido e ne restituiamo l'indice.
		if (supported_memory && supported_properties)
		{
			return i;
		}
	}

	return static_cast<uint32_t>(0);
}

VkShaderModule Utility::CreateShaderModule(const std::vector<char>& spirv_code)
{
	VkShaderModuleCreateInfo shader_module_create_info = {};
	shader_module_create_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.codeSize	= spirv_code.size();									 
	shader_module_create_info.pCode		= reinterpret_cast<const uint32_t*>(spirv_code.data());

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(m_MainDevice->LogicalDevice, &shader_module_create_info, nullptr, &shader_module);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Shader Module!");
	}

	return shader_module;
}


VkFormat Utility::ChooseSupportedFormat(const std::vector<VkFormat>& formats, 
	const VkImageTiling& tiling, const VkFormatFeatureFlags& feature_flags)
{
	for (VkFormat format : formats)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_MainDevice->PhysicalDevice, format, &properties);

		bool const is_tiling_linear		 = tiling == VK_IMAGE_TILING_LINEAR;
		bool const linear_support_flags	 = (properties.linearTilingFeatures & feature_flags) == feature_flags;
		bool const is_tiling_optimal	 = tiling == VK_IMAGE_TILING_OPTIMAL;
		bool const optimal_support_flags = (properties.optimalTilingFeatures & feature_flags) == feature_flags;
		
		if (is_tiling_linear && linear_support_flags)
		{
			return format;
		}
		else if (is_tiling_optimal && optimal_support_flags)
		{
			return format;
		}
	}
	throw std::runtime_error("Failed to find a matching format!");
}

void Utility::GetPossibleQueueFamilyIndices(const VkPhysicalDevice &possible_physical_device, QueueFamilyIndices& queue_family_indices)
{
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(possible_physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_family_list(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(possible_physical_device, &queue_family_count, queue_family_list.data());

	int queue_index = 0;

	if (queue_family_count > 0)
	{
		for (const auto& queue_family : queue_family_list)
		{
			// Se la QueueFamily è valida ed è una Queue grafica, salvo l'indice nel Renderer
			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queue_family_indices.GraphicsFamily = queue_index;
			}

			VkBool32 presentation_support = false;

			/*	Query per chiedere se è supportata la Presentation Mode nella Queue Family.
				Solitamente le Queue Family appartenenti alla grafica la supportano.
				Questa caratteristica è obbligatoria per presentare l'immagine dalla SwapChain alla Surface.*/
			vkGetPhysicalDeviceSurfaceSupportKHR(possible_physical_device, queue_index, *m_Surface, &presentation_support);

			// Se la Queue Family supporta la Presentation Mode, salvo l'indice nel Renderer (solitamente sono gli stessi indici)
			if (presentation_support)
			{
				queue_family_indices.PresentationFamily = queue_index;
			}

			if (queue_family_indices.isValid())
			{
				break;
			}

			++queue_index;
		}
	}
}

bool Utility::CheckPossibleDeviceExtensionSupport(const VkPhysicalDevice& possible_physical_device, const std::vector<const char*>& requested_device_extensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(possible_physical_device, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0)
		return false;

	std::vector<VkExtensionProperties> physicalDeviceExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(possible_physical_device, nullptr, &extensionCount, physicalDeviceExtensions.data());

	for (const auto& extensionToCheck : requested_device_extensions)
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
