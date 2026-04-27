#include "RenderPipelines.h"

#include "VulkanDevice.h"
#include "Mesh.h"
#include "Utils.h"
#include "BufferObjectsData.h"
#include "ResourceLimits.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// MaterialData se usa para el push constant range de la graphics pipeline.
// Si esta estructura cambia de tamanio, el range debe actualizarse.
struct MaterialData;

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------

void RenderPipelines::init( VulkanDevice& device,
                             VkRenderPass  mainRenderPass,
                             VkRenderPass  shadowRenderPass ) {
    _device          = &device;
    _mainRenderPass  = mainRenderPass;
    _shadowRenderPass = shadowRenderPass;

    // Orden obligatorio: layouts → pipeline layouts (dentro de cada pipeline) → pipelines.
    createDescriptorSetLayouts();

    // Cada createXxxPipeline crea su pipeline layout internamente
    // porque el layout y la pipeline son una unidad de configuracion.
    createGraphicsPipeline();
    createDeferredPipeline();
    createShadowPipeline();
    createComputePipeline();
    createObjectCullPipeline();
}

void RenderPipelines::destroy() {
    destroyPipelines();
    destroyPipelineLayouts();
    destroyDescriptorSetLayouts();

    _device           = nullptr;
    _mainRenderPass   = VK_NULL_HANDLE;
    _shadowRenderPass = VK_NULL_HANDLE;
}

// ---------------------------------------------------------------------------
// Descriptor set layouts
// ---------------------------------------------------------------------------

void RenderPipelines::createDescriptorSetLayouts() {
    createViewProjectionLayout();
    createTextureArrayLayout();
    createInputAttachmentsLayout();
    createDeferredDescriptorLayout();
    createComputeDescriptorLayout();
    createIndexedObjectsBufferLayout();
}

void RenderPipelines::createViewProjectionLayout() {
    // UBO de camara (view + proj). Visible en todos los stages graficos y en compute
    // porque lo usan tanto el vertex shader (transform) como el compute de culling.
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding            = 0;
    uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount    = 1;
    uboBinding.stageFlags         = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings    = &uboBinding;

    _viewProjectionLayout = _device->createDescriptorSetLayout( info );
}

void RenderPipelines::createTextureArrayLayout() {
    // Array de samplers de tamanio variable (hasta MAX_TEXTURES).
    // VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT permite que el
    // array ocupe solo los slots que realmente tienen textura cargada.
    constexpr int MAX_TEXTURES = ResourceLimits::MAX_TEXTURES;

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding            = 0;
    samplerBinding.descriptorCount    = MAX_TEXTURES;
    samplerBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.pImmutableSamplers = nullptr;
    samplerBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorBindingFlags flags =
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
    bindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlags.bindingCount  = 1;
    bindingFlags.pBindingFlags = &flags;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings    = &samplerBinding;
    info.pNext        = &bindingFlags;

    _textureArrayLayout = _device->createDescriptorSetLayout( info );
}

void RenderPipelines::createInputAttachmentsLayout() {
    // Los tres input attachments del GBuffer leidos en el subpass de iluminacion:
    //   binding 0 → albedo/color
    //   binding 1 → normales
    //   binding 2 → posicion en espacio mundo
    VkDescriptorSetLayoutBinding color{};
    color.binding         = 0;
    color.descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    color.descriptorCount = 1;
    color.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding normal{};
    normal.binding         = 1;
    normal.descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    normal.descriptorCount = 1;
    normal.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding position{};
    position.binding         = 2;
    position.descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    position.descriptorCount = 1;
    position.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = { color, normal, position };

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 3;
    info.pBindings    = bindings;

    _inputAttachmentsLayout = _device->createDescriptorSetLayout( info );
}

void RenderPipelines::createDeferredDescriptorLayout() {
    // Datos de iluminacion global usados en el subpass deferred:
    //   binding 3 → GlobalLighting UBO (eyePos, ambientVal)
    //   binding 6 → shadow map como COMBINED_IMAGE_SAMPLER
    // Los numeros de binding deben coincidir con el shader de deferred.
    VkDescriptorSetLayoutBinding lightingUbo{};
    lightingUbo.binding            = 3;
    lightingUbo.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingUbo.descriptorCount    = 1;
    lightingUbo.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    lightingUbo.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding shadowMapSampler{};
    shadowMapSampler.binding            = 6;
    shadowMapSampler.descriptorCount    = 1;
    shadowMapSampler.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowMapSampler.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadowMapSampler.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = { lightingUbo, shadowMapSampler };

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 2;
    info.pBindings    = bindings;

    _deferredDescriptorLayout = _device->createDescriptorSetLayout( info );
}

void RenderPipelines::createComputeDescriptorLayout() {
    // GlobalLighting UBO leido por el compute shader de light culling.
    //   binding 2 → GlobalLighting (eyePos, ambientVal)
    VkDescriptorSetLayoutBinding lightingUbo{};
    lightingUbo.binding            = 2;
    lightingUbo.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingUbo.descriptorCount    = 1;
    lightingUbo.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    lightingUbo.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings    = &lightingUbo;

    _computeDescriptorLayout = _device->createDescriptorSetLayout( info );
}

void RenderPipelines::createIndexedObjectsBufferLayout() {
    // Buffers de indices y objetos compartidos entre compute y fragment:
    //   binding 0 → index buffer (storage)
    //   binding 1 → object/light buffer (storage)
    VkDescriptorSetLayoutBinding indexBuffer{};
    indexBuffer.binding            = 0;
    indexBuffer.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indexBuffer.descriptorCount    = 1;
    indexBuffer.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    indexBuffer.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding objectBuffer{};
    objectBuffer.binding            = 1;
    objectBuffer.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objectBuffer.descriptorCount    = 1;
    objectBuffer.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    objectBuffer.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = { indexBuffer, objectBuffer };

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 2;
    info.pBindings    = bindings;

    _indexedObjectsBufferLayout = _device->createDescriptorSetLayout( info );
}

// ---------------------------------------------------------------------------
// Pipelines
// ---------------------------------------------------------------------------

void RenderPipelines::createGraphicsPipeline() {
    // Shaders: geometria al GBuffer (subpass 0 del main render pass).
    // Push constants: model matrix (vertex) + MaterialData (fragment).
    // Descriptor sets: ViewProjection (set 0), TextureArray (set 1).
    // 3 color attachments: albedo, normal, posicion.
    // Depth test habilitado y escribible.

    auto vertCode = readFile( "./shaders/build/vertex" );
    auto fragCode = readFile( "./shaders/build/fragment" );

    VkShaderModule vertModule = _device->createShaderModule( vertCode );
    VkShaderModule fragModule = _device->createShaderModule( fragCode );

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName  = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName  = "main";

    VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    auto bindingDesc   = Vertex::getBindingDescription();
    auto attributeDesc = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = 1;
    vertexInput.pVertexBindingDescriptions      = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>( attributeDesc.size() );
    vertexInput.pVertexAttributeDescriptions    = attributeDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Tres attachments de color: albedo, normales, posicion.
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable         = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendAttachmentState colorAttachments[] = {
        blendAttachment, blendAttachment, blendAttachment
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 3;
    colorBlending.pAttachments    = colorAttachments;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
    dynamicState.pDynamicStates    = dynamicStates.data();

    VkPushConstantRange modelMatrixRange{};
    modelMatrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelMatrixRange.size       = sizeof( glm::mat4 );
    modelMatrixRange.offset     = 0;

    VkPushConstantRange materialRange{};
    materialRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialRange.size       = sizeof( MaterialData );
    materialRange.offset     = sizeof( glm::mat4 );

    VkPushConstantRange pushRanges[] = { modelMatrixRange, materialRange };

    VkDescriptorSetLayout setLayouts[] = { _viewProjectionLayout, _textureArrayLayout };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 2;
    layoutInfo.pSetLayouts            = setLayouts;
    layoutInfo.pushConstantRangeCount = 2;
    layoutInfo.pPushConstantRanges    = pushRanges;

    _graphicsLayout = _device->createPipelineLayout( layoutInfo );

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = stages;
    pipelineInfo.pVertexInputState   = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = _graphicsLayout;
    pipelineInfo.renderPass          = _mainRenderPass;
    pipelineInfo.subpass             = 0;

    _graphicsPipeline = _device->createPipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

    _device->destroyShaderModule( fragModule );
    _device->destroyShaderModule( vertModule );
}

void RenderPipelines::createDeferredPipeline() {
    // Shaders: iluminacion diferida leyendo el GBuffer (subpass 1 del main render pass).
    // Sin vertex input: el quad de pantalla completa se genera en el vertex shader.
    // Sin push constants. Depth test en modo lectura (depthWriteEnable = false).
    // Descriptor sets: InputAttachments (set 0), ViewProjection (set 1),
    //                  IndexedObjectsBuffer (set 2), DeferredDescriptor (set 3).

    auto vertCode = readFile( "./shaders/build/readAttachmentVertex" );
    auto fragCode = readFile( "./shaders/build/deferred" );

    VkShaderModule vertModule = _device->createShaderModule( vertCode );
    VkShaderModule fragModule = _device->createShaderModule( fragCode );

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName  = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName  = "main";

    VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &blendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
    dynamicState.pDynamicStates    = dynamicStates.data();

    VkDescriptorSetLayout setLayouts[] = {
        _inputAttachmentsLayout,
        _viewProjectionLayout,
        _indexedObjectsBufferLayout,
        _deferredDescriptorLayout
    };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 4;
    layoutInfo.pSetLayouts    = setLayouts;

    _deferredLayout = _device->createPipelineLayout( layoutInfo );

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = stages;
    pipelineInfo.pVertexInputState   = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = _deferredLayout;
    pipelineInfo.renderPass          = _mainRenderPass;
    pipelineInfo.subpass             = 1;

    _deferredPipeline = _device->createPipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

    _device->destroyShaderModule( fragModule );
    _device->destroyShaderModule( vertModule );
}

void RenderPipelines::createShadowPipeline() {
    // Shader: solo vertex, sin fragment (depth-only pass).
    // Push constant: model matrix (vertex).
    // Descriptor set: ViewProjection (set 0) con la VP de la main light.
    // Sin color attachments. Depth test habilitado y escribible.
    // cull = NONE para evitar peter-panning en geometria fina.

    auto vertCode = readFile( "./shaders/build/shadowVertex" );
    VkShaderModule vertModule = _device->createShaderModule( vertCode );

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName  = "main";

    auto bindingDesc   = Vertex::getBindingDescription();
    auto attributeDesc = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = 1;
    vertexInput.pVertexBindingDescriptions      = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>( attributeDesc.size() );
    vertexInput.pVertexAttributeDescriptions    = attributeDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
    dynamicState.pDynamicStates    = dynamicStates.data();

    VkPushConstantRange modelMatrixRange{};
    modelMatrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelMatrixRange.size       = sizeof( glm::mat4 );
    modelMatrixRange.offset     = 0;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &_viewProjectionLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &modelMatrixRange;

    _shadowLayout = _device->createPipelineLayout( layoutInfo );

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 1;
    pipelineInfo.pStages             = &vertStage;
    pipelineInfo.pVertexInputState   = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = nullptr;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = _shadowLayout;
    pipelineInfo.renderPass          = _shadowRenderPass;
    pipelineInfo.subpass             = 0;

    _shadowPipeline = _device->createPipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

    _device->destroyShaderModule( vertModule );
}

void RenderPipelines::createComputePipeline() {
    // Shader: light culling en compute.
    // Descriptor sets: IndexedObjectsBuffer (set 0), ComputeDescriptor (set 1).
    // Sin render pass: es un compute shader puro.

    auto computeCode = readFile( "./shaders/build/lightCull" );
    VkShaderModule computeModule = _device->createShaderModule( computeCode );

    VkPipelineShaderStageCreateInfo computeStage{};
    computeStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeStage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    computeStage.module = computeModule;
    computeStage.pName  = "main";

    VkDescriptorSetLayout setLayouts[] = { _indexedObjectsBufferLayout, _computeDescriptorLayout };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 2;
    layoutInfo.pSetLayouts    = setLayouts;

    _computeLayout = _device->createPipelineLayout( layoutInfo );

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _computeLayout;
    pipelineInfo.stage  = computeStage;
    pipelineInfo.flags  = 0;
    pipelineInfo.pNext  = nullptr;

    _computePipeline = _device->createcomputePipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

    _device->destroyShaderModule( computeModule );
}

void RenderPipelines::createObjectCullPipeline() {
    // Shader: object culling en compute.
    // Descriptor sets: IndexedObjectsBuffer (set 0), ViewProjection (set 1).
    // Sin render pass: es un compute shader puro.

    auto computeCode = readFile( "./shaders/build/objectCull" );
    VkShaderModule computeModule = _device->createShaderModule( computeCode );

    VkPipelineShaderStageCreateInfo computeStage{};
    computeStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeStage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    computeStage.module = computeModule;
    computeStage.pName  = "main";

    VkDescriptorSetLayout setLayouts[] = { _indexedObjectsBufferLayout, _viewProjectionLayout };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 2;
    layoutInfo.pSetLayouts    = setLayouts;

    _objectCullLayout = _device->createPipelineLayout( layoutInfo );

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _objectCullLayout;
    pipelineInfo.stage  = computeStage;
    pipelineInfo.flags  = 0;
    pipelineInfo.pNext  = nullptr;

    _objectCullPipeline = _device->createcomputePipelines( VK_NULL_HANDLE, { pipelineInfo } )[0];

    _device->destroyShaderModule( computeModule );
}

// ---------------------------------------------------------------------------
// Destruccion
// ---------------------------------------------------------------------------

void RenderPipelines::destroyPipelines() {
    _device->destroyPipeline( _graphicsPipeline );
    _device->destroyPipeline( _deferredPipeline );
    _device->destroyPipeline( _shadowPipeline );
    _device->destroyPipeline( _computePipeline );
    _device->destroyPipeline( _objectCullPipeline );
}

void RenderPipelines::destroyPipelineLayouts() {
    _device->destroyPipelineLayout( _graphicsLayout );
    _device->destroyPipelineLayout( _deferredLayout );
    _device->destroyPipelineLayout( _shadowLayout );
    _device->destroyPipelineLayout( _computeLayout );
    _device->destroyPipelineLayout( _objectCullLayout );
}

void RenderPipelines::destroyDescriptorSetLayouts() {
    _device->destroyDescriptorSetLayout( _viewProjectionLayout );
    _device->destroyDescriptorSetLayout( _textureArrayLayout );
    _device->destroyDescriptorSetLayout( _inputAttachmentsLayout );
    _device->destroyDescriptorSetLayout( _deferredDescriptorLayout );
    _device->destroyDescriptorSetLayout( _computeDescriptorLayout );
    _device->destroyDescriptorSetLayout( _indexedObjectsBufferLayout );
}
