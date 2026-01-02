#include "Texture.h"
#include <vulkan/vulkan.h>
#include "App.h"
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
Texture::~Texture()
{
    //vkDestroySampler(device._device, textureSampler, nullptr);
    device.destroySampler( textureSampler );
    //vkDestroyImageView(device._device, textureImageView, nullptr);
    device.destroyImageView( textureImageView );
    //vkDestroyImage(device._device, textureImage, nullptr);
    device.destroyImage( textureImage );
    //vkFreeMemory(device._device, textureImageMemory, nullptr);
    device.freeMemory( textureImageMemory );
}
void Texture::loadTexture(const std::string& path, App& app)
 {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    int texWidth, texHeight, texChannels;
    unsigned char* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    //unsigned char* pixels = tinyimg_load(path.c_str(), &texWidth, &texHeight, TINYIMG_RGBA);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    //niveles de mipMapping
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    device.createBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    void* data;
    //vkMapMemory(device._device, stagingBufferMemory, 0, imageSize, 0, &data);
    device.mapMemory( stagingBufferMemory, 0, imageSize, &data );
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    //vkUnmapMemory(device._device, stagingBufferMemory);
    device.unmapMemory( stagingBufferMemory );

    stbi_image_free(pixels);

    createImage(texWidth, texHeight,mipLevels,VK_SAMPLE_COUNT_1_BIT, mFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT| VK_IMAGE_USAGE_SAMPLED_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    app.trasitionImageLayout(textureImage, mFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,mipLevels);
    app.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    app.trasitionImageLayout(textureImage, mFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,mipLevels);
    app.generateMipmaps(textureImage,mFormat, texWidth, texHeight, mipLevels);


    //vkDestroyBuffer(device._device, stagingBuffer, nullptr);
    device.destroyBuffer( stagingBuffer );
    //vkFreeMemory(device._device, stagingBufferMemory, nullptr);
    device.freeMemory( stagingBufferMemory );

    createImageView(mFormat,VK_IMAGE_ASPECT_COLOR_BIT,mipLevels);

    createTextureSampler();
}

void Texture::createImage(uint32_t width, uint32_t height, uint32_t mipLvl, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    texWidth = static_cast<uint32_t>(width);
    texHeight = static_cast<uint32_t>(height);
    mFormat = format;
    mipLevels = mipLvl;

    textureImage = device.createImage( texWidth, texHeight, mipLevels, numSamples, mFormat, tiling, usage, properties );

    textureImageMemory = device.bindMemoryToImage( textureImage );
}

void Texture::createImageView(VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    //TODO permitir niveles de mip mapping, mipLevels tiene que ser distinto de 1
    textureImageView = device.createImageView(textureImage, format, aspectFlags, mipLevels);
}

void Texture::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    //se podria configurar para cambiar el modo de repeticion
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    //VkPhysicalDeviceProperties properties{};
    //vkGetPhysicalDeviceProperties(device._physicalDevice, &properties);


    //samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0;
        //static_cast<float>(mipLevels);

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;

    //if (vkCreateSampler(device._device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create texture sampler!");
    //}

    textureSampler = device.createSampler( samplerInfo );
}
