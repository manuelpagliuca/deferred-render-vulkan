#include "pch.h"
#include "GraphicPipeline.h"

GraphicPipeline::GraphicPipeline()
{
	m_MainDevice	    = {};
	m_GraphicsPipeline  = 0;
	m_PipelineLayout	= 0;
	m_RenderPassHandler = new RenderPassHandler();
}

GraphicPipeline::GraphicPipeline(MainDevice &mainDevice, SwapChainHandler& swapChainHandler, RenderPassHandler * renderPass,
	VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSetLayout& textureSetLayout, VkPushConstantRange& pushCostantRange)
{
	m_MainDevice			= mainDevice;
	m_SwapChainHandler		= swapChainHandler;
	m_RenderPassHandler		= renderPass;
	m_DescriptorSetLayout	= descriptorSetLayout;
	m_TextureSetLayout		= textureSetLayout;
	m_PushCostantRange		= pushCostantRange;
	m_GraphicsPipeline		= 0;
	m_PipelineLayout		= 0;

	CreateGraphicPipeline();
}

void GraphicPipeline::CreatePipelineLayout()
{
	std::array<VkDescriptorSetLayout, 2> descriptoSetLayouts = { m_DescriptorSetLayout , m_TextureSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount			= static_cast<uint32_t>(descriptoSetLayouts.size());					  
	pipelineLayoutCreateInfo.pSetLayouts			= descriptoSetLayouts.data(); 
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;						  
	pipelineLayoutCreateInfo.pPushConstantRanges	= &m_PushCostantRange;		  
	
	VkResult res = vkCreatePipelineLayout(m_MainDevice.LogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);

	if (res != VK_SUCCESS)
		throw std::runtime_error("Failed to create Pipeline Layout.");
}

void GraphicPipeline::CreateGraphicPipeline()
{
	CreateShaderStages();
	CreateVertexInputStage();
	CreateInputAssemblyStage();
	CreateViewportScissorStage();
	CreateRasterizerStage();
	CreateMultisampleStage();
	CreateColourBlendingStage();
	CreateDepthStencilStage();
	CreatePipelineLayout();

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount				= 2;									// Numero di Shader Stages
	pipelineCreateInfo.pStages					= m_ShaderStages;						// Puntatore all'array di Shader Stages
	pipelineCreateInfo.pVertexInputState		= &m_VertexInputStage;					// Vertex Shader
	pipelineCreateInfo.pInputAssemblyState		= &m_InputAssemblyStage;				// Input Assembly 
	pipelineCreateInfo.pViewportState			= &m_ViewportScissorStage;				// ViewPort 
	pipelineCreateInfo.pDynamicState			= nullptr;								// Dynamic States 
	pipelineCreateInfo.pRasterizationState		= &m_Rasterizer;						// Rasterization
	pipelineCreateInfo.pMultisampleState		= &m_MultisampleStage;					// Multisampling 
	pipelineCreateInfo.pColorBlendState			= &m_ColourBlendingStage;				// Colout Blending 
	pipelineCreateInfo.pDepthStencilState		= &m_DepthStencilStage;					// Depth Stencil Test
	pipelineCreateInfo.layout					= m_PipelineLayout;						// Pipeline Layout
	pipelineCreateInfo.renderPass				= *m_RenderPassHandler->GetRenderPassReference();	// RenderPass Stage
	pipelineCreateInfo.subpass					= 0;									// Subpass utilizzati nel RenderPass
	pipelineCreateInfo.basePipelineHandle		= VK_NULL_HANDLE;						// Pipeline da cui derivare
	pipelineCreateInfo.basePipelineIndex		= -1;									// Indice della pipeline da cui derivare (in caso in cui siano passate molteplici)

	VkResult res = vkCreateGraphicsPipelines(m_MainDevice.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_GraphicsPipeline);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}

	DestroyShaderModules();
}

VkShaderModule GraphicPipeline::CreateShaderModules(const char* path)
{
	std::vector<char> shaderCode = Utility::ReadFile(path);
	return Utility::CreateShaderModule(shaderCode);
}

void GraphicPipeline::DestroyShaderModules()
{
	vkDestroyShaderModule(m_MainDevice.LogicalDevice, m_FragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_MainDevice.LogicalDevice, m_VertexShaderModule, nullptr);
}

void GraphicPipeline::SetVertexStageBindingDescription()
{
	m_VertexStageBindingDescription.binding   = 0;							// Indice di Binding (possono essere presenti molteplici stream di binding)
	m_VertexStageBindingDescription.stride	  = sizeof(Vertex);				// Scostamento tra gli elementi in input forniti dal Vertex
	m_VertexStageBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Rateo con cui i vertex attributes sono presi dai buffers TODO
																// VK_VERTEX_INPUT_RATE_VERTEX	 : Si sposta al prossimo vertice
																// VK_VERTEX_INPUT_RATE_INSTANCE : Si sposta al prossimo vertice per la prossima istanza
}

void GraphicPipeline::SetVertexttributeDescriptions()
{
	// Attribute Descriptions, come interpretare il Vertex Input su un dato binding stream

	// Attribute for the Vertex Position
	m_VertexStageAttributeDescriptions[0].binding  = 0;						    // Stream di binding al quale si riferisce
	m_VertexStageAttributeDescriptions[0].location = 0;						    // Locazione del binding nel Vertex Shader
	m_VertexStageAttributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT; // Formato e dimensione dell'attributo (32-bit per ogni canale di signed floating point)
	m_VertexStageAttributeDescriptions[0].offset   = offsetof(Vertex, pos);		// Offset della posizione all'interno della struct 'Vertex'

	// Attribute for the Vertex Color
	m_VertexStageAttributeDescriptions[1].binding  = 0;						    // Stream di binding al quale si riferisce
	m_VertexStageAttributeDescriptions[1].location = 1;						    // Locazione del binding nel Vertex Shader
	m_VertexStageAttributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT; // Formato e dimensione dell'attributo (32-bit per ogni canale di signed floating point)
	m_VertexStageAttributeDescriptions[1].offset   = offsetof(Vertex, col);		// Offset del colore all'interno della struct 'Vertex'

	// Attrribute for the textures
	m_VertexStageAttributeDescriptions[2].binding  = 0;						    // Stream di binding al quale si riferisce
	m_VertexStageAttributeDescriptions[2].location = 2;						    // Locazione del binding nel Vertex Shader
	m_VertexStageAttributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;		// Formato e dimensione dell'attributo (32-bit per ogni canale di signed floating point)
	m_VertexStageAttributeDescriptions[2].offset   = offsetof(Vertex, tex);		// Offset del colore all'interno della struct 'Vertex'
}

void GraphicPipeline::SetViewport()
{
	m_Viewport.x		= 0.f;														// Valore iniziale della viewport-x
	m_Viewport.y		= 0.f;														// Valore iniziale della viewport-y
	m_Viewport.width	= static_cast<float>(m_SwapChainHandler.GetExtentWidth());	// Larghezza della viewport
	m_Viewport.height	= static_cast<float>(m_SwapChainHandler.GetExtentHeight());	// Altezza della viewport
	m_Viewport.minDepth = 0.0f;														// Minima profondità del framebuffer
	m_Viewport.maxDepth = 1.0f;														// Massima profondità del framebuffer
}

void GraphicPipeline::SetScissor()
{
	m_Scissor.offset = { 0,0 };			  // Offset da cui iniziare a tagliare la regione
	m_Scissor.extent = m_SwapChainHandler.GetExtent(); // Extent che descrive la regione da tagliare
}

void GraphicPipeline::CreateShaderStages()
{
	m_ShaderStages[0] = CreateVertexShaderStage();
	m_ShaderStages[1] = CreateFragmentShaderStage();
}

void GraphicPipeline::CreateVertexInputStage()
{
	SetVertexStageBindingDescription();
	SetVertexttributeDescriptions();

	m_VertexInputStage = {};
	m_VertexInputStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_VertexInputStage.vertexBindingDescriptionCount = 1;												 	 // Numero di Vertex Binding Descriptions
	m_VertexInputStage.pVertexBindingDescriptions = &m_VertexStageBindingDescription;								 // Puntatore ad un array di Vertex Binding Description
	m_VertexInputStage.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VertexStageAttributeDescriptions.size()); // Numero di Attribute Descriptions
	m_VertexInputStage.pVertexAttributeDescriptions = m_VertexStageAttributeDescriptions.data();
}

void GraphicPipeline::CreateInputAssemblyStage()
{
	m_InputAssemblyStage = {};
	m_InputAssemblyStage.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_InputAssemblyStage.topology				= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Tipo della primitiva con cui assemblare i vertici
	m_InputAssemblyStage.primitiveRestartEnable = VK_FALSE;							  // Abilità di ripartire con una nuova strip
}

void GraphicPipeline::CreateViewportScissorStage()
{
	SetViewport();
	SetScissor();
	
	m_ViewportScissorStage.sType		 = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_ViewportScissorStage.viewportCount = 1;
	m_ViewportScissorStage.pViewports	 = &m_Viewport;
	m_ViewportScissorStage.scissorCount  = 1;			
	m_ViewportScissorStage.pScissors	 = &m_Scissor;	
}

void GraphicPipeline::CreateDynamicStatesStage()
{
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
}

void GraphicPipeline::CreateRasterizerStage()
{
	m_Rasterizer.sType					 = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_Rasterizer.depthClampEnable		 = VK_FALSE;						// Evita il depth-clamping del far-plane, quando un oggetto va al di fuori del farplane 
																			// viene schiacciato su quest'ultimo anzichè non venir caricato.
	m_Rasterizer.rasterizerDiscardEnable = VK_FALSE;						// Se abilitato scarta tutti i dati e non crea alcun fragment 
																			// viene utilizzato per pipeline dove si devono recuperare i dati (dal geometry e tess) e non disegnare niente 
	m_Rasterizer.polygonMode			 = VK_POLYGON_MODE_FILL;			// Modalità di colorazione della primitiva
	m_Rasterizer.lineWidth				 = 1.0f;							// Thickness delle linee delle primitive
	m_Rasterizer.cullMode				 = VK_CULL_MODE_BACK_BIT;			// Quale faccia del triangolo non disegnare (la parte dietro non visibile)
	m_Rasterizer.frontFace				 = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Rispetto da dove stiamo guardando il triangolo, se i punti sono stati disegnati in ordine orario allora stiamo guardano la primitiva da di fronte
	m_Rasterizer.depthBiasEnable		 = VK_FALSE;						// Depth Bias tra i fragments, evita la problematica della Shadow Acne
}

void GraphicPipeline::CreateMultisampleStage()
{
	m_MultisampleStage.sType				= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_MultisampleStage.sampleShadingEnable  = VK_FALSE;				 // Disabilita il SampleShading
	m_MultisampleStage.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Numero di sample utilizzati per fragment
}

void GraphicPipeline::CreateColourBlendingStage()
{
	// #7 Stage - BLENDING
	// Decide come mescolare un nuovo colore in un fragment, utilizzando il vecchio valore
	// Equazione di blending [LERP] = (srcColour*newColour)colorBlendOp(dstColourBlendFactor * oldColour)
	m_ColourStateAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_ColourStateAttachment.blendEnable	   = VK_TRUE;	// Abilita il blending

	// blending [LERP] = (VK_BLEND_FACTOR_SRC_ALPHA*newColour)colorBlendOp(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColour)
	//			[LERP] = (newColourAlpha * newColour) + ((1-newColourAlpha) * oldColour)
	m_ColourStateAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			 // srcColor -> Componente Alpha
	m_ColourStateAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // dstColor -> 1 - Componente Alpha
	m_ColourStateAttachment.colorBlendOp		= VK_BLEND_OP_ADD;					 // Operazione di blending (somma)

	// (1*newAlpha) + (0*oldAlpha) = newAlpha
	m_ColourStateAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Moltiplica il nuovo alpha value per 1
	m_ColourStateAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Moltiplica il vecchio alpha value per 0
	m_ColourStateAttachment.alphaBlendOp		= VK_BLEND_OP_ADD;	  // Potremmo utilizzare anche una sottrazione ed il risultato sarebbe il medesimo

	m_ColourBlendingStage.sType			  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_ColourBlendingStage.logicOpEnable   = VK_FALSE;			   // Disabilita l'alternativa di utilizzare le operazioni logiche al posto dei calcoli aritmetici
	m_ColourBlendingStage.attachmentCount = 1;					   // Numero di attachments
	m_ColourBlendingStage.pAttachments	  = &m_ColourStateAttachment; // Puntatore ad un array di attachments
}

void GraphicPipeline::CreateDepthStencilStage()
{
	m_DepthStencilStage.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_DepthStencilStage.depthTestEnable			= VK_TRUE;
	m_DepthStencilStage.depthWriteEnable		= VK_TRUE;
	m_DepthStencilStage.depthCompareOp			= VK_COMPARE_OP_LESS;
	m_DepthStencilStage.depthBoundsTestEnable	= VK_FALSE;
	m_DepthStencilStage.stencilTestEnable		= VK_FALSE;
}

VkPipelineShaderStageCreateInfo GraphicPipeline::CreateVertexShaderStage()
{
	m_VertexShaderModule = CreateShaderModules(m_VertexShaderSPIRVPath);

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = m_VertexShaderModule;
	vertexShaderCreateInfo.pName  = "main";

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
}