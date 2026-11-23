#include "GBuffer.h"
#include "VulkanDevice.h"
#include "VulkanWindow.h"
#include <array>
#include <stdexcept>

void GBuffer::init( VulkanDevice& device, VulkanWindow& window, VkRenderPass renderPass )
{
    _device     = &device;
    _window     = &window;
    _renderPass = renderPass;
}

void GBuffer::create()
{
    if ( _device == nullptr ) {
        throw std::runtime_error( "GBuffer::create() called before init()" );
    }

    destroy();

    _extent = _window->getExtent();

    createColorResources    ( _extent );
    createNormalResources   ( _extent );
    createPositionResources ( _extent );
    createDepthResources    ( _extent );


    _colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    _colorAttachment.clearValue = VkClearValue{ .color = { 0.0f, 0.0f, 0.0f, 1.0f } };
    _colorAttachment.imageLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ;
    _colorAttachment.imageView = _colorTexture->textureImageView;
    _colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    _colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    _colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    _colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    _colorAttachment.resolveImageView = VK_NULL_HANDLE;
    _colorAttachment.pNext = NULL;

    _normalAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    _normalAttachment.clearValue = VkClearValue{ .color = { 0.0f, 0.0f, 0.0f, 1.0f } };
    _normalAttachment.imageLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ;
    _normalAttachment.imageView = _normalTexture->textureImageView;
    _normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    _normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    _normalAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    _normalAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    _normalAttachment.resolveImageView = VK_NULL_HANDLE;
    _normalAttachment.pNext = NULL;

    _positionAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    _positionAttachment.clearValue = VkClearValue{ .color = { 0.0f, 0.0f, 0.0f, 1.0f } };
    _positionAttachment.imageLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ;
    _positionAttachment.imageView = _posTexture->textureImageView;
    _positionAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    _positionAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    _positionAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    _positionAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    _positionAttachment.resolveImageView = VK_NULL_HANDLE;
    _positionAttachment.pNext = NULL;
}

void GBuffer::destroy()
{

    _colorTexture.reset();
    _normalTexture.reset();
    _posTexture.reset();
    _depthTexture.reset();
}

void GBuffer::createColorResources( VkExtent2D extent )
{
    const VkFormat colorFormat = _window->getSwapChainFormat();

    _colorTexture = std::make_unique<Texture>( *_device );
    _colorTexture->createImage( extent.width, extent.height, 1,
                                VK_SAMPLE_COUNT_1_BIT,
                                colorFormat,
                                VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT     |
                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    _colorTexture->createImageView( colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
}

void GBuffer::createNormalResources( VkExtent2D extent )
{
    const VkFormat normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    _normalTexture = std::make_unique<Texture>( *_device );
    _normalTexture->createImage( extent.width, extent.height, 1,
                                 VK_SAMPLE_COUNT_1_BIT,
                                 normalFormat,
                                 VK_IMAGE_TILING_OPTIMAL,
                                 VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT     |
                                 VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    _normalTexture->createImageView( normalFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
}

void GBuffer::createPositionResources( VkExtent2D extent )
{
    const VkFormat positionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    _posTexture = std::make_unique<Texture>( *_device );
    _posTexture->createImage( extent.width, extent.height, 1,
                              VK_SAMPLE_COUNT_1_BIT,
                              positionFormat,
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT     |
                              VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    _posTexture->createImageView( positionFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
}

void GBuffer::createDepthResources( VkExtent2D extent )
{
    const VkFormat depthFormat = _device->findDepthFormat();

    _depthTexture = std::make_unique<Texture>( *_device );
    _depthTexture->createImage( extent.width, extent.height, 1,
                                VK_SAMPLE_COUNT_1_BIT,
                                depthFormat,
                                VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    _depthTexture->createImageView( depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1 );
}
