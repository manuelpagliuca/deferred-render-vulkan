#include "pch.h"
#include "GraphicPipeline.h"

GraphicPipeline::GraphicPipeline()
{
	m_MainDevice	    = {};
	m_GraphicsPipeline  = 0;
	m_PipelineLayout	= 0;
	m_RenderPassHandler = RenderPassHandler();
}

GraphicPipeline::GraphicPipeline(MainDevice &mainDevice, SwapChainHandler& swapChainHandler, RenderPassHandler &renderPass, VkDescriptorSetLayout& descriptorSetLayout,
	TextureObjects& textureObjects, VkPushConstantRange& pushCostantRange)
{
	m_MainDevice			= mainDevice;
	m_SwapChainHandler		= swapChainHandler;
	m_RenderPassHandler		= renderPass;
	m_DescriptorSetLayout	= descriptorSetLayout;
	m_TextureObjects		= textureObjects;
	m_PushCostantRange		= pushCostantRange;

	m_GraphicsPipeline  = 0;
	m_PipelineLayout    = 0;

	CreateGraphicPipeline();
}

void GraphicPipeline::CreateGraphicPipeline()
{
	CreateShaderStages();
	/* Vertex Input Binding Description per i Vertex */
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;							// Indice di Binding (possono essere presenti molteplici stream di binding)
	bindingDescription.stride = sizeof(Vertex);				// Scostamento tra gli elementi in input forniti dal Vertex
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Rateo con cui i vertex attributes sono presi dai buffers TODO
																// VK_VERTEX_INPUT_RATE_VERTEX	 : Si sposta al prossimo vertice
																// VK_VERTEX_INPUT_RATE_INSTANCE : Si sposta al prossimo vertice per la prossima istanza

	// Attribute Descriptions, come interpretare il Vertex Input su un dato binding stream
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

	// Attribute for the Vertex Position
	attributeDescriptions[0].binding = 0;						    // Stream di binding al quale si riferisce
	attributeDescriptions[0].location = 0;						    // Locazione del binding nel Vertex Shader
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Formato e dimensione dell'attributo (32-bit per ogni canale di signed floating point)
	attributeDescriptions[0].offset = offsetof(Vertex, pos);		// Offset della posizione all'interno della struct 'Vertex'

	// Attribute for the Vertex Color
	attributeDescriptions[1].binding = 0;						    // Stream di binding al quale si riferisce
	attributeDescriptions[1].location = 1;						    // Locazione del binding nel Vertex Shader
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // Formato e dimensione dell'attributo (32-bit per ogni canale di signed floating point)
	attributeDescriptions[1].offset = offsetof(Vertex, col);		// Offset del colore all'interno della struct 'Vertex'

	// Attrribute for the textures
	attributeDescriptions[2].binding = 0;						    // Stream di binding al quale si riferisce
	attributeDescriptions[2].location = 2;						    // Locazione del binding nel Vertex Shader
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;		// Formato e dimensione dell'attributo (32-bit per ogni canale di signed floating point)
	attributeDescriptions[2].offset = offsetof(Vertex, tex);		// Offset del colore all'interno della struct 'Vertex'

	/* Graphic Pipeline */

	// #1 STAGE - VERTEX INPUT
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;												 	 // Numero di Vertex Binding Descriptions
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;								 // Puntatore ad un array di Vertex Binding Description
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); // Numero di Attribute Descriptions
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();						 // Puntatore ad un array di Attribute Descriptions

	// #2 Stage - INPUT ASSEMBLY
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Tipo della primitiva con cui assemblare i vertici
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;							  // Abilità di ripartire con una nuova strip

	// #3 Stage - VIEWPORT & SCISSOR

	// Viewport
	VkViewport viewport = {};
	viewport.x = 0.f;											// Valore iniziale della viewport-x
	viewport.y = 0.f;											// Valore iniziale della viewport-y
	viewport.width = static_cast<float>(m_SwapChainHandler.GetExtentWidth());	// Larghezza della viewport
	viewport.height = static_cast<float>(m_SwapChainHandler.GetExtentHeight());	// Altezza della viewport
	viewport.minDepth = 0.0f;											// Minima profondità del framebuffer
	viewport.maxDepth = 1.0f;											// Massima profondità del framebuffer

	// Scissor
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };			  // Offset da cui iniziare a tagliare la regione
	scissor.extent = m_SwapChainHandler.GetExtent(); // Extent che descrive la regione da tagliare

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;			// Numero di Viewport
	viewportStateCreateInfo.pViewports = &viewport;  // Puntatore ad un array di viewport
	viewportStateCreateInfo.scissorCount = 1;			// Numero di Scissor
	viewportStateCreateInfo.pScissors = &scissor;	// Puntatore ad un array di scissor

	// #4 Stage - DYNAMIC STATES
	/*

	// Stati dinamici da abilitare (permettere la resize senza ridefinire la pipeline)
	// N.B. Fare una resize non comporta la modifca delle extent delle tue immagini, quindi per effetuarla
	// correttamente si necessita di ricostruire una SwapChain nuova.
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // Dynamic Viewport : Permette di utilizzare il comando di resize (vkCmdSetViewport(commandBuffer, 0, 1, &viewport) ...
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);  // Dynamic Scissorr : Permette la resize da un commandbuffer vkCmdSetScissor(commandBuffer, 0, 1, &viewport) ...


	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType			 = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO; // Tipo di struttura
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());	 //
	dynamicStateCreateInfo.pDynamicStates	 = dynamicStateEnables.data();

	*/

	// #5 Stage - RASTERIZER
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;						// Evita il depth-clamping del far-plane, quando un oggetto va al di fuori del farplane 
																					// viene schiacciato su quest'ultimo anzichè non venir caricato.
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;						// Se abilitato scarta tutti i dati e non crea alcun fragment 
																					// viene utilizzato per pipeline dove si devono recuperare i dati (dal geometry e tess) e non disegnare niente 
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;			// Modalità di colorazione della primitiva
	rasterizerCreateInfo.lineWidth = 1.0f;							// Thickness delle linee delle primitive
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;			// Quale faccia del triangolo non disegnare (la parte dietro non visibile)
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Rispetto da dove stiamo guardando il triangolo, se i punti sono stati disegnati in ordine orario allora stiamo guardano la primitiva da di fronte
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;						// Depth Bias tra i fragments, evita la problematica della Shadow Acne

	// #6 Stage - MULTISAMPLING
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;				// Disabilita il SampleShading
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Numero di sample utilizzati per fragment

	// #7 Stage - BLENDING
	// Decide come mescolare un nuovo colore in un fragment, utilizzando il vecchio valore
	// Equazione di blending [LERP] = (srcColour*newColour)colorBlendOp(dstColourBlendFactor * oldColour)
	VkPipelineColorBlendAttachmentState colourStateAttachment = {};
	colourStateAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | // Colori su cui applicare (write) il blending
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	colourStateAttachment.blendEnable = VK_TRUE;	// Abilita il blending

	// blending [LERP] = (VK_BLEND_FACTOR_SRC_ALPHA*newColour)colorBlendOp(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColour)
	//			[LERP] = (newColourAlpha * newColour) + ((1-newColourAlpha) * oldColour)
	colourStateAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			 // srcColor -> Componente Alpha
	colourStateAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // dstColor -> 1 - Componente Alpha
	colourStateAttachment.colorBlendOp = VK_BLEND_OP_ADD;					 // Operazione di blending (somma)

	// (1*newAlpha) + (0*oldAlpha) = newAlpha
	colourStateAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Moltiplica il nuovo alpha value per 1
	colourStateAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Moltiplica il vecchio alpha value per 0
	colourStateAttachment.alphaBlendOp = VK_BLEND_OP_ADD;	  // Potremmo utilizzare anche una sottrazione ed il risultato sarebbe il medesimo

	VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
	colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendingCreateInfo.logicOpEnable = VK_FALSE;			   // Disabilita l'alternativa di utilizzare le operazioni logiche al posto dei calcoli aritmetici
	colourBlendingCreateInfo.attachmentCount = 1;					   // Numero di attachments
	colourBlendingCreateInfo.pAttachments = &colourStateAttachment; // Puntatore ad un array di attachments

	/* Pipeline Layout */
	std::array<VkDescriptorSetLayout, 2> descriptoSetLayouts = { m_DescriptorSetLayout , m_TextureObjects.SamplerSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptoSetLayouts.size());					  // Numero di DescriptorSet
	pipelineLayoutCreateInfo.pSetLayouts = descriptoSetLayouts.data(); // Puntatore ad una lista di DescriptorSetLayout
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;						  // Numero di PushCostants
	pipelineLayoutCreateInfo.pPushConstantRanges = &m_PushCostantRange;		  // Puntatore ad un array di PushCostants

	VkResult res = vkCreatePipelineLayout(m_MainDevice.LogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout.");
	}

	/* DEPTH STENCIL TESTING */
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;


	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = CreateShaderModules(m_VertexShaderSPIRVPath);
	vertexShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = CreateShaderModules(m_FragmentShaderSPIRVPath);
	fragmentShaderCreateInfo.pName = "main";


	// Creazione della pipeline
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;						 // Numero di Shader Stages
	pipelineCreateInfo.pStages = m_ShaderStages;				 // Puntatore all'array di Shader Stages
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;	 // Vertex Shader
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;	 // Input Assembly 
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;	 // ViewPort 
	pipelineCreateInfo.pDynamicState = nullptr;					 // Dynamic States 
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;	 // Rasterization
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;	 // Multisampling 
	pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo; // Colout Blending 
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;   // Depth Stencil Test
	pipelineCreateInfo.layout = m_PipelineLayout;			 // Pipeline Layout
	pipelineCreateInfo.renderPass = m_RenderPassHandler.GetRenderPass();		 // RenderPass Stage
	pipelineCreateInfo.subpass = 0;						 // Subpass utilizzati nel RenderPass

	// Le derivate delle pipeline permettono di derivare da un altra pipeline (soluzione ottimale)
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Pipeline da cui derivare
	pipelineCreateInfo.basePipelineIndex = -1;			  // Indice della pipeline da cui derivare (in caso in cui siano passate molteplici)

	res = vkCreateGraphicsPipelines(m_MainDevice.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_GraphicsPipeline);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}

	DestroyShaderModules();
}

VkShaderModule GraphicPipeline::CreateShaderModules(const char* path)
{
	std::vector<char> shaderCode = Utility::ReadFile(path);
	return Utility::CreateShaderModule(m_MainDevice.LogicalDevice, shaderCode);
}

void GraphicPipeline::DestroyShaderModules()
{
	vkDestroyShaderModule(m_MainDevice.LogicalDevice, m_FragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_MainDevice.LogicalDevice, m_VertexShaderModule, nullptr);
}

void GraphicPipeline::CreateShaderStages()
{
	VkPipelineShaderStageCreateInfo vertexStage = CreateVertexShaderStage();
	VkPipelineShaderStageCreateInfo fragmentStage = CreateFragmentShaderStage();

	m_ShaderStages[0] = vertexStage;
	m_ShaderStages[1] = fragmentStage;
}

VkPipelineShaderStageCreateInfo GraphicPipeline::CreateVertexShaderStage()
{
	m_VertexShaderModule = CreateShaderModules(m_VertexShaderSPIRVPath);

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = m_VertexShaderModule;
	vertexShaderCreateInfo.pName = "main";

	return vertexShaderCreateInfo;
}

VkPipelineShaderStageCreateInfo GraphicPipeline::CreateFragmentShaderStage()
{
	m_FragmentShaderModule = CreateShaderModules(m_FragmentShaderSPIRVPath);

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = m_FragmentShaderModule;
	fragmentShaderCreateInfo.pName  = "main";
	
	return fragmentShaderCreateInfo;
}

void GraphicPipeline::DestroyPipeline()
{
	vkDestroyPipeline(m_MainDevice.LogicalDevice, m_GraphicsPipeline, nullptr);

	vkDestroyPipelineLayout(m_MainDevice.LogicalDevice, m_PipelineLayout, nullptr);

	vkDestroyRenderPass(m_MainDevice.LogicalDevice, m_RenderPassHandler.GetRenderPass(), nullptr);
}