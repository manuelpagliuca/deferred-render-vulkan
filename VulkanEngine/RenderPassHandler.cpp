#include "pch.h"

#include "RenderPassHandler.h"

RenderPassHandler::RenderPassHandler()
{
	m_RenderPass = 0;
}

RenderPassHandler::RenderPassHandler(MainDevice* maindevice, SwapChainHandler* swapChainHandler)
{
	m_MainDevice		= maindevice;
	m_SwapChainHandler	= swapChainHandler;
}

void RenderPassHandler::CreateRenderPass()
{
	SetColourAttachment(m_SwapChainHandler->GetSwapChainImageFormat());
	SetDepthAttachment();
	SetSubpassDescription();
	SetSubpassDependencies();

	std::array<VkAttachmentDescription, 2> renderPassAttachments = { m_ColourAttachment, m_DepthAttachment };

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount	= static_cast<uint32_t>(renderPassAttachments.size());	// Numero di attachment presenti nel Renderpass
	renderPassCreateInfo.pAttachments		= renderPassAttachments.data();							// Puntatore all'array di attachments
	renderPassCreateInfo.subpassCount		= 1;													// Numero di Subpasses coinvolti
	renderPassCreateInfo.pSubpasses			= &m_SubpassDescription;								// Puntatore all'array di Subpass
	renderPassCreateInfo.dependencyCount	= static_cast<uint32_t>(m_SubpassDependencies.size());  // Numero di Subpass Dependencies coinvolte
	renderPassCreateInfo.pDependencies		= m_SubpassDependencies.data();							// Puntatore all'array/vector di Subpass Dependencies

	VkResult res = vkCreateRenderPass(m_MainDevice->LogicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Render Pass!");
	}
}

void RenderPassHandler::SetColourAttachment(const VkFormat &imageFormat)
{
	/* Attachment del RenderPass */
	// Definizione del Layout iniziale e Layout finale del RenderPass
	m_ColourAttachment.flags		  = 0;									// Proprietà addizionali degli attachment
	m_ColourAttachment.format		  = imageFormat;						// Formato dell'immagini utilizzato nella SwapChain
	m_ColourAttachment.samples		  = VK_SAMPLE_COUNT_1_BIT;				// Numero di samples da utilizzare per l'immagine (multisampling)
	m_ColourAttachment.loadOp		  = VK_ATTACHMENT_LOAD_OP_CLEAR;		// Dopo aver caricato il colourAttachment performerà un operazione (pulisce l'immagine)
	m_ColourAttachment.storeOp		  = VK_ATTACHMENT_STORE_OP_STORE;		// Al teermine dell'opperazione di RenderPass si esegue un operazione di store dei dati
	m_ColourAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Descrive cosa fare con lo stencil prima del rendering
	m_ColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Descrive cosa fare con lo stencil dopo l'ultimo subpass, quindi dopo il rendering

	// FrameBuffer data vengono salvate come immagini, ma alle immagini è possibile 
	// fornire differenti layout dei dati, questo per motivi di ottimizzazione nell'uso di alcune operazioni.
	// Un determinato formato può essere più adatto epr una particolare operazione, per esempio uno shader
	// legge i dati in un formato particolare, ma quando presentiamo quei dati allo schermo quel formato è differente
	// perchè deve essere presentato in una certa maniera allo schermo.
	// È presente un layout intermedio che verrà svolto dai subpasses
	m_ColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Layout dell'immagini prima del RenderPass sarà non definito
	m_ColourAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Layout delle immagini dopo il RenderPass, l'immagine avrà un formato presentabile
}

void RenderPassHandler::SetDepthAttachment()
{
	std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
	m_DepthAttachment.format			= Utility::ChooseSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	m_DepthAttachment.samples			= VK_SAMPLE_COUNT_1_BIT;
	m_DepthAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	m_DepthAttachment.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	m_DepthAttachment.stencilLoadOp		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	m_DepthAttachment.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	m_DepthAttachment.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
	m_DepthAttachment.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

void RenderPassHandler::SetSubpassDependencies()
{
	// VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	m_SubpassDependencies[0].srcSubpass			= VK_SUBPASS_EXTERNAL;					 // La dipendenza ha un ingresso esterno (indice)
	m_SubpassDependencies[0].srcStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;  // Indica lo stage dopo il quale accadrà la conversione di layout (dopo il termine del Subpass esterno)
	m_SubpassDependencies[0].srcAccessMask		= VK_ACCESS_MEMORY_READ_BIT;			 // L'operazione (read) che deve essere fatta prima di effettuare la conversione.

	m_SubpassDependencies[0].dstSubpass			= 0;											  // La dipendenza ha come destinazione il primo SubPass della lista 'subpass'
	m_SubpassDependencies[0].dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // Indica lo stage prima del quale l'operazione deve essere avvenuta (Output del final color).
	m_SubpassDependencies[0].dstAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;	  // Indica le operazioni prima delle quali la conversione (read e write) deve essere già avvenuta
												// (Questi access mask indicano gli stage dove accadono)
	m_SubpassDependencies[0].dependencyFlags	= 0;											  // Disabita eventuali flags inerenti all'esecuzione e memoria

	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	m_SubpassDependencies[1].srcSubpass			= 0;											  // Indice del primo Subpass
	m_SubpassDependencies[1].srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // La conversione deve avvenire dopo l'output del final color
	m_SubpassDependencies[1].srcAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // La conversione deve avvenire dopo lettura/scrittura

	m_SubpassDependencies[1].dstSubpass			= VK_SUBPASS_EXTERNAL;				   // La dipendenza ha come destinazione l'esterno
	m_SubpassDependencies[1].dstStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // La conversione deve avvenire prima della fine della pipeline
	m_SubpassDependencies[1].dstAccessMask		= VK_ACCESS_MEMORY_READ_BIT;			   // La conversione deve avvenire prima della lettura
	m_SubpassDependencies[1].dependencyFlags	= 0;
}

void RenderPassHandler::DestroyRenderPass()
{
	vkDestroyRenderPass(m_MainDevice->LogicalDevice, m_RenderPass, nullptr);
}

void RenderPassHandler::SetSubpassDescription()
{
	/* SubPass */
	// Per passare l'attachment al Subpass si utilizza l'AttachmentReference.
	// È un tipo di dato che funge da riferimento/indice nella lista 'colourAttachment' definita
	// nella createInfo del RenderPass.

	// Colour Attachment Reference
	m_ColourAttachmentReference.attachment	= 0;										// L'indice dell'elemento nella lista "colourAttachment" definita nel RenderPass.
	m_ColourAttachmentReference.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Formato ottimale per il colorAttachment (conversione implicita UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL)

	// Depth Attachment Reference
	m_DepthAttachmentReference.attachment	= 1;
	m_DepthAttachmentReference.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	m_SubpassDescription.pipelineBindPoint			= VK_PIPELINE_BIND_POINT_GRAPHICS;	// Tipo di Pipeline supportato dal Subpass (Pipeline Grafica)
	m_SubpassDescription.colorAttachmentCount		= 1;								// Numero di ColourAttachment utilizzati.
	m_SubpassDescription.pColorAttachments			= &m_ColourAttachmentReference;		// Puntatore ad un array di colorAttachment
	m_SubpassDescription.pDepthStencilAttachment	= &m_DepthAttachmentReference;		// Disabita eventuali flags inerenti all'esecuzione e memoria
}