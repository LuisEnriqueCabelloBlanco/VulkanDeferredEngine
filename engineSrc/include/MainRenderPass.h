#pragma once

#include <vulkan/vulkan.h>

class VulkanDevice;
class VulkanWindow;

// ---------------------------------------------------------------------------
// MainRenderPass
// ---------------------------------------------------------------------------
/*
  Propietario del VkRenderPass principal del motor: el pase deferred de dos
  subpasses que escribe al GBuffer y resuelve el lighting.

  ESTRUCTURA DEL RENDER PASS
  --------------------------
  Attachment 0 — present output    (swapchain format, COLOR_ATTACHMENT_OPTIMAL → PRESENT_SRC)
  Attachment 1 — depth/stencil     (device depth format)
  Attachment 2 — GBuffer albedo    (swapchain format, transient)
  Attachment 3 — GBuffer normals   (R16G16B16A16_SFLOAT, transient)
  Attachment 4 — GBuffer position  (R16G16B16A16_SFLOAT, transient)

  Subpass 0 — Geometry:  escribe a attachments 2, 3, 4 con depth en 1.
  Subpass 1 — Lighting:  lee 2, 3, 4 como input attachments; escribe a 0.

  CICLO DE VIDA
  -------------
  init()    — crea el VkRenderPass. Necesita el device y la ventana para
              consultar el swapchain format y el depth format.
              Debe llamarse antes de GBuffer, RenderPipelines y DescriptorManager.
  destroy() — destruye el VkRenderPass.
              Debe llamarse en cleanup() antes de cerrar el device.

  RESIZE
  ------
  El render pass NO necesita recrearse en un resize normal porque su
  estructura (formatos, subpasses, dependencias) no depende de la resolución.
  Solo debe recrearse si el formato de la swapchain cambia, lo cual es raro
  pero posible. RenderEngine ya gestiona ese caso en recreateSwapChain():

      if ( _window.getSwapChainFormat() != oldFormat ) {
          _mainRenderPass.destroy();
          _mainRenderPass.init( _device, _window );
      }

  El GBuffer sí se recrea en cada resize (sus texturas tienen resolución fija),
  pero como recibe el VkRenderPass como parámetro de construcción y solo lo
  usa para crear framebuffers, simplemente seguirá usando el mismo handle.
*/
class MainRenderPass {
public:
    MainRenderPass()  = default;
    ~MainRenderPass() = default;

    MainRenderPass( const MainRenderPass& ) = delete;
    MainRenderPass& operator=( const MainRenderPass& ) = delete;

    void init( VulkanDevice& device, VulkanWindow& window );
    void destroy();

    VkRenderPass get() const { return _renderPass; }
    operator VkRenderPass() const { return _renderPass; }

private:
    VulkanDevice* _device      = nullptr;
    VkRenderPass  _renderPass  = VK_NULL_HANDLE;
};
