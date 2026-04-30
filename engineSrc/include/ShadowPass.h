#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "Texture.h"

class VulkanDevice;

/**
 * ShadowPass gestiona los recursos necesarios para el mapeo de sombras.
 * Responsabilidades:
 * - Crear y destruir la textura de profundidad (Shadow Map).
 * - Gestionar el Framebuffer y el RenderPass propios del shadow pass.
 * - Mantener las dimensiones del mapa de sombras, independientes de la ventana.
 *
 * CICLO DE VIDA
 * -------------
 * init()    — asocia las dependencias externas (device, extent).
 *             Debe llamarse una vez antes del primer create().
 * create()  — crea todos los recursos GPU. Llama a destroy() internamente,
 *             por lo que es seguro repetirla si el extent cambia.
 * destroy() — libera todos los recursos GPU.
 */
class ShadowPass {
public:
    ShadowPass()  = default;
    ~ShadowPass() { destroy(); }

    ShadowPass( const ShadowPass& ) = delete;
    ShadowPass& operator=( const ShadowPass& ) = delete;

    void init( VulkanDevice& device, VkExtent2D extent );
    void create();
    void destroy();

    VkRenderPass   getRenderPass()  const { return _renderPass;       }
    const Texture* getShadowMap()   const { return _shadowMap.get();  }
    VkFramebuffer  getFramebuffer() const { return _framebuffer;      }
    VkExtent2D     getExtent()      const { return _extent;           }

private:
    void createRenderPass();
    void createShadowMap();
    void createFramebuffer();

    VulkanDevice* _device     = nullptr;
    VkExtent2D    _extent     = {};
    VkRenderPass  _renderPass = VK_NULL_HANDLE;

    std::unique_ptr<Texture> _shadowMap;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
};
