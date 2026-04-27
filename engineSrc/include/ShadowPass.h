#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "Texture.h"

class VulkanDevice;

/**
 * ShadowPass gestiona los recursos necesarios para el mapeo de sombras (Shadow Mapping).
 * Responsabilidades:
 * - Crear y destruir la textura de profundidad (Shadow Map).
 * - Gestionar el Framebuffer específico para el renderizado desde la luz.
 * - Mantener las dimensiones (Extent) de la sombra, independientes de la ventana.
 */
class ShadowPass {
public:
    ShadowPass(VulkanDevice& device, VkExtent2D extent);
    ~ShadowPass();

    void create();  // Inicializa recursos
    void destroy(); // Libera recursos

    // Getters para el RenderEngine
    VkRenderPass getRenderPass() const { return _renderPass; }
    const Texture* getShadowMap() const { return _shadowMap.get(); }
    VkFramebuffer getFramebuffer() const { return _framebuffer; }
    VkExtent2D getExtent() const { return _extent; }

private:
    // Sub-metodos de create()
    void createRenderPass();
    void createShadowMap();
    void createFramebuffer();

    VulkanDevice& _device;
    VkRenderPass _renderPass = VK_NULL_HANDLE;  // RenderPass de sombras (solo profundidad)
    VkExtent2D _extent;                         // Resolución del mapa de sombras (ej. 2048x2048)

    std::unique_ptr<Texture> _shadowMap;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
};