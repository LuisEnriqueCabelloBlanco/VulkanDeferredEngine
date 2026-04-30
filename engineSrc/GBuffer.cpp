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
    createFramebuffers      ( _extent );
}

void GBuffer::destroy()
{
    for ( VkFramebuffer fb : _framebuffers ) {
        if ( fb != VK_NULL_HANDLE ) {
            _device->destroyFrameBuffer( fb );
        }
    }
    _framebuffers.clear();

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

void GBuffer::createFramebuffers( VkExtent2D extent )
{
    const auto& imageViews = _window->getImageViews();
    _framebuffers.resize( imageViews.size() );

    for ( size_t i = 0; i < imageViews.size(); ++i ) {
        std::array<VkImageView, 5> attachments = {
            imageViews[i],
            _depthTexture->textureImageView,
            _colorTexture->textureImageView,
            _normalTexture->textureImageView,
            _posTexture->textureImageView,
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>( attachments.size() );
        framebufferInfo.pAttachments    = attachments.data();
        framebufferInfo.width           = extent.width;
        framebufferInfo.height          = extent.height;
        framebufferInfo.layers          = 1;

        _framebuffers[i] = _device->createFrameBuffer( framebufferInfo );
    }
}
