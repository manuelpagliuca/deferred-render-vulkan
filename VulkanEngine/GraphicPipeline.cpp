#include "pch.h"
#include "GraphicPipeline.h"

GraphicPipeline::GraphicPipeline()
{
	m_MainDevice			= {};
	m_FirstPipeline			= 0;
	m_FirstPipelineLayout	= 0;
	m_RenderPassHandler		= new RenderPassHandler();
}

GraphicPipeline::GraphicPipeline(MainDevice* main_device, SwapChain* swap_chain, RenderPassHandler* render_pass_handler)
{
	m_MainDevice		= main_device;
	m_SwapChain			= swap_chain;
	m_RenderPassHandler = render_pass_handler;
}

void GraphicPipeline::CreateGraphicPipeline()
{
	//CreateShaderStages();
	m_ShaderStages[0] = CreateVertexShaderStage("./Shaders/vert.spv");
	m_ShaderStages[1] = CreateFragmentShaderStage("./Shaders/frag.spv");

	// How the data for a single vertex (including info such as position, colour, texture coords, normals, etc) is as a whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding		= 0;									// Can bind multiple streams of data, this defines which one
	bindingDescription.stride		= sizeof(Vertex);						// Size of a single vertex object
	bindingDescription.inputRate	= VK_VERTEX_INPUT_RATE_VERTEX;		// How to move between data after each vertex.
																	// VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex
																	// VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance

	// How the data for an attribute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions;

	// Position Attribute
	attributeDescriptions[0].binding	= 0;							// Which binding the data is at (should be same as above)
	attributeDescriptions[0].location	= 0;							// Location in shader where data will be read from
	attributeDescriptions[0].format		= VK_FORMAT_R32G32B32_SFLOAT;	// Format the data will take (also helps define size of data)
	attributeDescriptions[0].offset		= offsetof(Vertex, pos);		// Where this attribute is defined in the data for a single vertex

	// Colour Attribute
	attributeDescriptions[1].binding	= 0;
	attributeDescriptions[1].location	= 1;
	attributeDescriptions[1].format		= VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset		= offsetof(Vertex, col);

	// Normal Attribute
	attributeDescriptions[2].binding	= 0;
	attributeDescriptions[2].location	= 2;
	attributeDescriptions[2].format		= VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset		= offsetof(Vertex, nrm);

	// Texture Attribute
	attributeDescriptions[3].binding	= 0;
	attributeDescriptions[3].location	= 3;
	attributeDescriptions[3].format		= VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset		= offsetof(Vertex, tex);

	// -- VERTEX INPUT --
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount		= 1;
	vertexInputCreateInfo.pVertexBindingDescriptions		= &bindingDescription;											// List of Vertex Binding Descriptions (data spacing/stride information)
	vertexInputCreateInfo.vertexAttributeDescriptionCount	= static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions		= attributeDescriptions.data();								// List of Vertex Attribute Descriptions (data format and where to bind to/from)

	// -- INPUT ASSEMBLY --
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;		// Primitive type to assemble vertices as
	inputAssembly.primitiveRestartEnable	= VK_FALSE;					// Allow overriding of "strip" topology to start new primitives

	// -- VIEWPORT & SCISSOR (Dimensioni del framebuffer) --
	VkViewport viewport = {};
	viewport.x			= 0.0f;									
	viewport.y			= 0.0f;									
	viewport.width		= static_cast<float>(m_SwapChain->GetExtentWidth());	
	viewport.height		= static_cast<float>(m_SwapChain->GetExtentHeight());	
	viewport.minDepth	= 0.0f;							
	viewport.maxDepth	= 1.0f;							

	// Create a scissor info struct
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };							
	scissor.extent = m_SwapChain->GetExtent();

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount	= 1;
	viewportStateCreateInfo.pViewports		= &viewport;
	viewportStateCreateInfo.scissorCount	= 1;
	viewportStateCreateInfo.pScissors		= &scissor;

	// -- RASTERIZER --
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable			= VK_FALSE;							// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
	rasterizerCreateInfo.rasterizerDiscardEnable	= VK_FALSE;							// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
	rasterizerCreateInfo.polygonMode				= VK_POLYGON_MODE_FILL;				// How to handle filling points between vertices
	rasterizerCreateInfo.lineWidth					= 1.0f;								// How thick lines should be when drawn
	rasterizerCreateInfo.cullMode					= VK_CULL_MODE_BACK_BIT;			// Which face of a tri to cull
	rasterizerCreateInfo.frontFace					= VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Winding to determine which side is front
	rasterizerCreateInfo.depthBiasEnable			= VK_FALSE;							// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

	// -- MULTISAMPLING --
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable		= VK_FALSE;					// Enable multisample shading or not
	multisamplingCreateInfo.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

	// -- BLENDING --
	// Blending decides how to blend a new colour being written to a fragment, with the old value

	// Blend Attachment State (how blending is handled)
	VkPipelineColorBlendAttachmentState colourState = {};
	colourState.colorWriteMask	=	VK_COLOR_COMPONENT_R_BIT |	// Colori su cui applicare il blending
									VK_COLOR_COMPONENT_G_BIT | 
									VK_COLOR_COMPONENT_B_BIT | 
									VK_COLOR_COMPONENT_A_BIT;
	colourState.blendEnable		= VK_FALSE;													// Enable blending

	// Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
	colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colourState.colorBlendOp		= VK_BLEND_OP_ADD;

	// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
	//			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

	colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourState.alphaBlendOp		= VK_BLEND_OP_ADD;
	// Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

	std::array<VkPipelineColorBlendAttachmentState, 3> colourStates
	{
		colourState,
		colourState,
		colourState
	};

	VkPipelineColorBlendStateCreateInfo colour_blending = {};
	colour_blending.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blending.logicOpEnable		= VK_FALSE;				// Alternative to calculations is to use logical operations
	colour_blending.attachmentCount		= static_cast<uint32_t>(colourStates.size());
	colour_blending.pAttachments		= colourStates.data();

	// -- PIPELINE LAYOUT --
	std::array<VkDescriptorSetLayout, 2> desc_set_layouts =
	{
		m_ViewProjectionSetLayout,
		m_TextureSetLayout,
	};

	VkPipelineLayoutCreateInfo fst_pipeline_layout = {};
	fst_pipeline_layout.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	fst_pipeline_layout.setLayoutCount			= static_cast<uint32_t>(desc_set_layouts.size());
	fst_pipeline_layout.pSetLayouts				= desc_set_layouts.data();
	fst_pipeline_layout.pushConstantRangeCount	= 1;
	fst_pipeline_layout.pPushConstantRanges		= &m_PushCostantRange;

	VkResult result = vkCreatePipelineLayout(m_MainDevice->LogicalDevice, &fst_pipeline_layout, nullptr, &m_FirstPipelineLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}

	// -- DEPTH STENCIL TESTING --
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
	depth_stencil_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info.depthTestEnable			= VK_TRUE;				// Enable checking depth to determine fragment write
	depth_stencil_info.depthWriteEnable			= VK_TRUE;				// Enable writing to depth buffer (to replace old values)
	depth_stencil_info.depthCompareOp			= VK_COMPARE_OP_LESS;	// Comparison operation that allows an overwrite (is in front)
	depth_stencil_info.depthBoundsTestEnable	= VK_FALSE;				// Depth Bounds Test: Does the depth value exist between two bounds
	depth_stencil_info.stencilTestEnable		= VK_FALSE;				// Enable Stencil Test

	// -- GRAPHICS PIPELINE CREATION --
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount			= 2;							
	pipeline_info.pStages				= m_ShaderStages;				
	pipeline_info.pVertexInputState		= &vertexInputCreateInfo;		
	pipeline_info.pInputAssemblyState	= &inputAssembly;
	pipeline_info.pViewportState		= &viewportStateCreateInfo;
	pipeline_info.pDynamicState			= nullptr;
	pipeline_info.pRasterizationState	= &rasterizerCreateInfo;
	pipeline_info.pMultisampleState		= &multisamplingCreateInfo;
	pipeline_info.pColorBlendState		= &colour_blending;
	pipeline_info.pDepthStencilState	= &depth_stencil_info;
	pipeline_info.layout				= m_FirstPipelineLayout;									
	pipeline_info.renderPass			= *m_RenderPassHandler->GetOffScreenRenderPassReference();
	pipeline_info.subpass				= 0;											
	pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;	
	pipeline_info.basePipelineIndex		= -1;				

	result = vkCreateGraphicsPipelines(m_MainDevice->LogicalDevice, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_FirstPipeline);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}

	DestroyShaderModules();

	// -SECOND PIPELINE-
	m_ShaderStages[0] = CreateVertexShaderStage("./Shaders/second_vert.spv");
	m_ShaderStages[1] = CreateFragmentShaderStage("./Shaders/second_frag.spv");

	vertexInputCreateInfo.vertexBindingDescriptionCount		= 0;
	vertexInputCreateInfo.pVertexBindingDescriptions		= nullptr;
	vertexInputCreateInfo.vertexAttributeDescriptionCount	= 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions		= nullptr;

	colour_blending.attachmentCount							= 1;
	colour_blending.pAttachments							= &colourState;

	rasterizerCreateInfo.cullMode							= VK_CULL_MODE_FRONT_BIT;
	rasterizerCreateInfo.frontFace							= VK_FRONT_FACE_COUNTER_CLOCKWISE;

	depth_stencil_info.depthWriteEnable						= VK_FALSE;

	std::array<VkDescriptorSetLayout, 3> snd_pipeline_desc_set_layouts =
	{
		m_InputSetLayout,
		m_LightSetLayout,
		m_SettingsSetLayout
	};

	// Create new pipeline layout
	VkPipelineLayoutCreateInfo snd_pipeline_layout = {};
	snd_pipeline_layout.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	snd_pipeline_layout.setLayoutCount			= static_cast<uint32_t>(snd_pipeline_desc_set_layouts.size());
	snd_pipeline_layout.pSetLayouts				= snd_pipeline_desc_set_layouts.data();
	snd_pipeline_layout.pushConstantRangeCount	= 0;
	snd_pipeline_layout.pPushConstantRanges		= nullptr;

	result = vkCreatePipelineLayout(m_MainDevice->LogicalDevice, &snd_pipeline_layout, nullptr, &m_SecondPipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Pipeline Layout!");
	}

	pipeline_info.pStages		= m_ShaderStages;			// Update second shader stage list
	pipeline_info.layout		= m_SecondPipelineLayout;	
	pipeline_info.subpass		= 0;
	pipeline_info.renderPass	= *m_RenderPassHandler->GetRenderPassReference();

	result = vkCreateGraphicsPipelines(m_MainDevice->LogicalDevice, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_SecondPipeline);

	if (result != VK_SUCCESS)
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
	vkDestroyShaderModule(m_MainDevice->LogicalDevice, m_FragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_MainDevice->LogicalDevice, m_VertexShaderModule, nullptr);
}

void GraphicPipeline::SetDescriptorSetLayouts(
	VkDescriptorSetLayout& descriptorSetLayout, 
	VkDescriptorSetLayout& textureObjects, 
	VkDescriptorSetLayout& inputSetLayout,
	VkDescriptorSetLayout& light_set_layout,
	VkDescriptorSetLayout& settings_set_layout)
{
	m_ViewProjectionSetLayout	= descriptorSetLayout;
	m_TextureSetLayout			= textureObjects;
	m_InputSetLayout			= inputSetLayout;
	m_LightSetLayout			= light_set_layout;
	m_SettingsSetLayout			= settings_set_layout;

	m_FirstPipeline				= 0;
	m_FirstPipelineLayout		= 0;

	m_SecondPipeline			= 0;
	m_SecondPipelineLayout		= 0;
}

void GraphicPipeline::SetPushCostantRange(VkPushConstantRange& pushCostantRange)
{
	m_PushCostantRange = pushCostantRange;
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
	m_Viewport.width	= static_cast<float>(m_SwapChain->GetExtentWidth());	// Larghezza della viewport
	m_Viewport.height	= static_cast<float>(m_SwapChain->GetExtentHeight());	// Altezza della viewport
	m_Viewport.minDepth = 0.0f;														// Minima profondità del framebuffer
	m_Viewport.maxDepth = 1.0f;														// Massima profondità del framebuffer
}

void GraphicPipeline::SetScissor()
{
	m_Scissor.offset = { 0,0 };			  // Offset da cui iniziare a tagliare la regione
	m_Scissor.extent = m_SwapChain->GetExtent(); // Extent che descrive la regione da tagliare
}
/*
void GraphicPipeline::CreateShaderStages()
{
	m_ShaderStages[0] = CreateVertexShaderStage();
	m_ShaderStages[1] = CreateFragmentShaderStage();
}*/
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

VkPipelineShaderStageCreateInfo GraphicPipeline::CreateVertexShaderStage(const char* vertex_str)
{
	m_VertexShaderModule = CreateShaderModules(vertex_str);

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = m_VertexShaderModule;
	vertexShaderCreateInfo.pName  = "main";

	return vertexShaderCreateInfo;
}

VkPipelineShaderStageCreateInfo GraphicPipeline::CreateFragmentShaderStage(const char* frag_str)
{
	m_FragmentShaderModule = CreateShaderModules(frag_str);

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = m_FragmentShaderModule;
	fragmentShaderCreateInfo.pName  = "main";
	
	return fragmentShaderCreateInfo;
}

void GraphicPipeline::DestroyPipeline()
{
	vkDestroyPipeline(m_MainDevice->LogicalDevice, m_FirstPipeline, nullptr);
	vkDestroyPipelineLayout(m_MainDevice->LogicalDevice, m_FirstPipelineLayout, nullptr);

	vkDestroyPipeline(m_MainDevice->LogicalDevice, m_SecondPipeline, nullptr);
	vkDestroyPipelineLayout(m_MainDevice->LogicalDevice, m_SecondPipelineLayout, nullptr);
}