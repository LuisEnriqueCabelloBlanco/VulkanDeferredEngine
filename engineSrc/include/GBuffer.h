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
 *
 * CICLO DE VIDA
 * -------------
 * init()    — asocia las dependencias externas (device, window, renderPass).
 *             Debe llamarse una vez antes del primer create().
 * create()  — crea o recrea todos los recursos GPU.
 *             Llama a destroy() internamente antes de crear, por lo que es
 *             seguro llamarla varias veces (resize).
 * destroy() — libera todos los recursos GPU. Los punteros de dependencia
 *             permanecen válidos para un create() posterior.
 */
class GBuffer {
public:
    GBuffer()  = default;
    ~GBuffer() { destroy(); }

    GBuffer( const GBuffer& ) = delete;
    GBuffer& operator=( const GBuffer& ) = delete;

    void init( VulkanDevice& device, VulkanWindow& window, VkRenderPass renderPass );
    void create();
    void destroy();

    // Getters de texturas para la fase de iluminación
    const Texture* getColorTexture()  const { return _colorTexture.get();  }
    const Texture* getNormalTexture() const { return _normalTexture.get(); }
    const Texture* getPosTexture()    const { return _posTexture.get();    }
    const Texture* getDepthTexture()  const { return _depthTexture.get();  }

    const std::vector<VkFramebuffer>& getFramebuffers() const { return _framebuffers; }
    VkExtent2D getExtent() const { return _extent; }

private:
    void createColorResources    ( VkExtent2D extent );
    void createNormalResources   ( VkExtent2D extent );
    void createPositionResources ( VkExtent2D extent );
    void createDepthResources    ( VkExtent2D extent );
    void createFramebuffers      ( VkExtent2D extent );

    VulkanDevice* _device      = nullptr;
    VulkanWindow* _window      = nullptr;
    VkRenderPass  _renderPass  = VK_NULL_HANDLE;
    VkExtent2D    _extent      = {};

    std::unique_ptr<Texture> _depthTexture;
    std::unique_ptr<Texture> _colorTexture;
    std::unique_ptr<Texture> _normalTexture;
    std::unique_ptr<Texture> _posTexture;

    std::vector<VkFramebuffer> _framebuffers;
};
