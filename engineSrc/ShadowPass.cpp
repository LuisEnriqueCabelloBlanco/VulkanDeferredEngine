#include "ShadowPass.h"
#include "VulkanDevice.h"
#include <array>
#include <stdexcept>

void ShadowPass::init( VulkanDevice& device, VkExtent2D extent )
{
    _device = &device;
    _extent = extent;
}

void ShadowPass::create()
{
    if ( _device == nullptr ) {
        throw std::runtime_error( "ShadowPass::create() called before init()" );
    }

    destroy();

    createRenderPass();
    createShadowMap();
    createFramebuffer();
}

void ShadowPass::destroy()
{
    if ( _framebuffer != VK_NULL_HANDLE ) {
        _device->destroyFrameBuffer( _framebuffer );
        _framebuffer = VK_NULL_HANDLE;
    }

    if ( _renderPass != VK_NULL_HANDLE ) {
        _device->destroyRenderPass( _renderPass );
        _renderPass = VK_NULL_HANDLE;
    }

    _shadowMap.reset();
}

void ShadowPass::createRenderPass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = _device->findDepthFormat();
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 0;
    subpass.pColorAttachments       = nullptr;
    subpass.pDepthStencilAttachment = &depthRef;

    std::array<VkAttachmentDescription, 1> attachments = { depthAttachment };

    VkRenderPassCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = static_cast<uint32_t>( attachments.size() );
    info.pAttachments    = attachments.data();
    info.subpassCount    = 1;
    info.pSubpasses      = &subpass;
    info.dependencyCount = 0;
    info.pDependencies   = nullptr;

    _renderPass = _device->createRenderPass( info );
}

void ShadowPass::createShadowMap()
{
    _shadowMap = std::make_unique<Texture>( *_device );

    _shadowMap->createImage(
        _extent.width,
        _extent.height,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        _device->findDepthFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    _shadowMap->createImageView( _device->findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1 );
    _shadowMap->createTextureSampler( VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER );
}

void ShadowPass::createFramebuffer()
{
    std::array<VkImageView, 1> attachments = {
        _shadowMap->textureImageView
    };

    VkFramebufferCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass      = _renderPass;
    info.attachmentCount = static_cast<uint32_t>( attachments.size() );
    info.pAttachments    = attachments.data();
    info.width           = _extent.width;
    info.height          = _extent.height;
    info.layers          = 1;

    _framebuffer = _device->createFrameBuffer( info );
}
