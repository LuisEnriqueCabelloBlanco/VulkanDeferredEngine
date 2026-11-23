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

    createShadowMap();
    createRenderPass();
    //createFramebuffer();
}

void ShadowPass::destroy()
{
    _shadowMap.reset();
}

void ShadowPass::startPass(VkCommandBuffer commandBuffer)
{
    _device->transitionImageLayout(commandBuffer,
        _shadowMap->textureImage, _device->findDepthFormat(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        1);

    vkCmdBeginRendering(commandBuffer, &_renderingInfo);
}

void ShadowPass::endPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRendering(commandBuffer);

    _device->transitionImageLayout(commandBuffer,
        _shadowMap->textureImage, _device->findDepthFormat(),
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
        1);
}

void ShadowPass::createRenderPass()
{
    //inicializacion del atachment de profundidad
    _depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    _depthAttachment.clearValue = VkClearValue{ .depthStencil = { 1.0f,0 } };
    _depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    _depthAttachment.imageView = _shadowMap->textureImageView;
    _depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    _depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    _depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    _depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    _depthAttachment.resolveImageView = VK_NULL_HANDLE;
    _depthAttachment.pNext = NULL;


    //Inicializacion de la rendering info
    _renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    _renderingInfo.renderArea.offset = { 0,0 };
    _renderingInfo.layerCount = 1;
    _renderingInfo.renderArea.extent = _extent;
    _renderingInfo.colorAttachmentCount = 0;
    _renderingInfo.pColorAttachments = NULL;
    _renderingInfo.pDepthAttachment = &_depthAttachment;
    _renderingInfo.pStencilAttachment = NULL;
    _renderingInfo.flags = 0;
    _renderingInfo.viewMask = 0;
    _renderingInfo.pNext = NULL;

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
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    _shadowMap->createImageView( _device->findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1 );
    _shadowMap->createTextureSampler( VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER );

    //_device->singleTimeTransitionImageLayout(_shadowMap.get()->textureImage, _shadowMap.get()->mFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1);

}

