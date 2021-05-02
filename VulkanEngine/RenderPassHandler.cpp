#include "pch.h"

#include "RenderPassHandler.h"

RenderPassHandler::RenderPassHandler()
{
	m_RenderPass = 0;
}

RenderPassHandler::RenderPassHandler(MainDevice& maindevice, SwapChainHandler& swapChainHandler)
{
	CreateRenderPass(maindevice, swapChainHandler);
}

void RenderPassHandler::CreateRenderPass(MainDevice& maindevice, SwapChainHandler& swapChainHandler)
{
	/* Attachment del RenderPass */
	// Definizione del Layout iniziale e Layout finale del RenderPass
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.flags = 0;								// Proprietà addizionali degli attachment
	colourAttachment.format = swapChainHandler.GetSwapChainImageFormat();			// Formato dell'immagini utilizzato nella SwapChain
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;			// Numero di samples da utilizzare per l'immagine (multisampling)
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;		// Dopo aver caricato il colourAttachment performerà un operazione (pulisce l'immagine)
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;		// Al teermine dell'opperazione di RenderPass si esegue un operazione di store dei dati
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // Descrive cosa fare con lo stencil prima del rendering
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Descrive cosa fare con lo stencil dopo l'ultimo subpass, quindi dopo il rendering

	// FrameBuffer data vengono salvate come immagini, ma alle immagini è possibile 
	// fornire differenti layout dei dati, questo per motivi di ottimizzazione nell'uso di alcune operazioni.
	// Un determinato formato può essere più adatto epr una particolare operazione, per esempio uno shader
	// legge i dati in un formato particolare, ma quando presentiamo quei dati allo schermo quel formato è differente
	// perchè deve essere presentato in una certa maniera allo schermo.
	// È presente un layout intermedio che verrà svolto dai subpasses
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		// Layout dell'immagini prima del RenderPass sarà non definito
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Layout delle immagini dopo il RenderPass, l'immagine avrà un formato presentabile


	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = Utility::ChooseSupportedFormat(maindevice, { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/* SubPass */
	// Per passare l'attachment al Subpass si utilizza l'AttachmentReference.
	// È un tipo di dato che funge da riferimento/indice nella lista 'colourAttachment' definita
	// nella createInfo del RenderPass.
	VkAttachmentReference colourAttachmentReference = {};
	colourAttachmentReference.attachment = 0;										 // L'indice dell'elemento nella lista "colourAttachment" definita nel RenderPass.
	colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Formato ottimale per il colorAttachment (conversione implicita UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL)

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Tipo di Pipeline supportato dal Subpass (Pipeline Grafica)
	subpass.colorAttachmentCount = 1;								// Numero di ColourAttachment utilizzati.
	subpass.pColorAttachments = &colourAttachmentReference;		// Puntatore ad un array di colorAttachment
	subpass.pDepthStencilAttachment = &depthAttachmentReference;



	/* SubPass Dependencies */
	std::array<VkSubpassDependency, 2> subpassDependencies;

	// VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;					 // La dipendenza ha un ingresso esterno (indice)
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Indica lo stage dopo il quale accadrà la conversione di layout (dopo il termine del Subpass esterno)
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;			 // L'operazione (read) che deve essere fatta prima di effettuare la conversione.

	subpassDependencies[0].dstSubpass = 0;											  // La dipendenza ha come destinazione il primo SubPass della lista 'subpass'
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Indica lo stage prima del quale l'operazione deve essere avvenuta (Output del final color).
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |		  // Indica le operazioni prima delle quali la conversione (read e write) deve essere già avvenuta
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;		  // (Questi access mask indicano gli stage dove accadono)
	subpassDependencies[0].dependencyFlags = 0;											  // Disabita eventuali flags inerenti all'esecuzione e memoria

	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	subpassDependencies[1].srcSubpass = 0;											  // Indice del primo Subpass
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // La conversione deve avvenire dopo l'output del final color
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |		  // La conversione deve avvenire dopo lettura/scrittura
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;				   // La dipendenza ha come destinazione l'esterno
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // La conversione deve avvenire prima della fine della pipeline
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;			   // La conversione deve avvenire prima della lettura
	subpassDependencies[1].dependencyFlags = 0;									   // Disabita eventuali flags inerenti all'esecuzione e memoria

	std::array<VkAttachmentDescription, 2> renderPassAttachments = { colourAttachment, depthAttachment };

	/* Render Pass */
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());												  // Numero di attachment presenti nel Renderpass
	renderPassCreateInfo.pAttachments = renderPassAttachments.data();					  // Puntatore all'array di attachments
	renderPassCreateInfo.subpassCount = 1;												  // Numero di Subpasses coinvolti
	renderPassCreateInfo.pSubpasses = &subpass;										  // Puntatore all'array di Subpass
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size()); // Numero di Subpass Dependencies coinvolte
	renderPassCreateInfo.pDependencies = subpassDependencies.data();						  // Puntatore all'array/vector di Subpass Dependencies

	VkResult res = vkCreateRenderPass(maindevice.LogicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Render Pass!");
	}
}
