#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "Texture.h"

class VulkanDevice;
class VulkanWindow;

/**
 * GBuffer gestiona el Geometry Buffer para renderizado diferido.
 * Responsabilidades:
 * - Crear texturas de Albedo, Normales, Posiciones y Profundidad de escena.
 * - Mantener un Framebuffer por cada imagen de la Swapchain (sincronizado con la ventana).
 */
class GBuffer {
public:
    GBuffer(VulkanDevice& device, VulkanWindow& window, VkRenderPass renderPass);
    ~GBuffer();

    void create();
    void destroy();

    // Getters de texturas para la fase de iluminaci�n
    const Texture* getColorTexture() const { return _colorTexture.get(); }
    const Texture* getNormalTexture() const { return _normalTexture.get(); }
    const Texture* getPosTexture() const { return _posTexture.get(); }
    const Texture* getDepthTexture() const { return _depthTexture.get(); }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return _framebuffers; }
    VkExtent2D getExtent() const { return _extent; }

private:
    void createColorResources(VkExtent2D extent);
    void createNormalResources(VkExtent2D extent);
    void createPositionResources(VkExtent2D extent);
    void createDepthResources(VkExtent2D extent);
    void createFramebuffers(VkExtent2D extent);

    VulkanDevice& _device;
    VulkanWindow& _window;
    VkRenderPass _renderPass;
    VkExtent2D _extent;

    std::unique_ptr<Texture> _depthTexture;
    std::unique_ptr<Texture> _colorTexture;
    std::unique_ptr<Texture> _normalTexture;
    std::unique_ptr<Texture> _posTexture;

    std::vector<VkFramebuffer> _framebuffers;
};