#include "ShadowPass.h"
#include "VulkanDevice.h"
#include <array>

ShadowPass::ShadowPass(VulkanDevice& device, VkExtent2D extent) :
    _device(device),
    _extent(extent)
{}

ShadowPass::~ShadowPass()
{
    destroy();
}

void ShadowPass::create()
{
    // Limpiar antes de crear
    destroy();  

    // Primero creamos el Render Pass
    createRenderPass();

    createShadowMap();

    createFramebuffer();
}

void ShadowPass::destroy() {
    if (_framebuffer != VK_NULL_HANDLE) {
        _device.destroyFrameBuffer(_framebuffer);
        _framebuffer = VK_NULL_HANDLE;
    }

    if (_renderPass != VK_NULL_HANDLE) {
        _device.destroyRenderPass(_renderPass);
        _renderPass = VK_NULL_HANDLE;
    }

    _shadowMap.reset();
}

void ShadowPass::createRenderPass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = _device.findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;


    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pColorAttachments = NULL;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkAttachmentDescription, 1> attachments = { depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = NULL;


    _renderPass = _device.createRenderPass(renderPassInfo);
}

void ShadowPass::createShadowMap()
{
    _shadowMap = std::make_unique<Texture>(_device);

    // Para las sombras necesitamos SAMPLED_BIT para poder leerla luego en el shader de iluminación
    _shadowMap->createImage(
        _extent.width,
        _extent.height,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        _device.findDepthFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    // Crear el Image View
    _shadowMap->createImageView(_device.findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    // shadowMap necesita sampler porque el shader de lighting lo lee como COMBINED_IMAGE_SAMPLER
    _shadowMap->createTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
}

void ShadowPass::createFramebuffer()
{
    std::array<VkImageView, 1> attachments = {
    _shadowMap->textureImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = _renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = _extent.width;
    framebufferInfo.height = _extent.height;
    framebufferInfo.layers = 1;

    _framebuffer = _device.createFrameBuffer(framebufferInfo);
}
