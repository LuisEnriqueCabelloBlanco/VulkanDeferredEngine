#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <iostream>

// ---------------------------------------------------------------------------
// VulkanInstance
// ---------------------------------------------------------------------------
/*
  Propietario del VkInstance y del VkDebugUtilsMessengerEXT.

  Encapsula todo lo que existe antes de que haya un device o una superficie:
    - Enumeración y validación de capas de validación.
    - Enumeración de extensiones requeridas (SDL + debug utils).
    - Creación del VkInstance.
    - Creación y destrucción del debug messenger.

  El par create/destroy de las funciones de extensión
  (CreateDebugUtilsMessengerEXT / DestroyDebugUtilsMessengerEXT) también vive
  aquí porque son funciones de instancia, no de device.

  LO QUE NO GESTIONA ESTA CLASE
  ------------------------------
  Surface, device, swapchain: son pasos posteriores que dependen de la
  instancia pero tienen su propio ciclo de vida.
*/
class VulkanInstance {
public:
    VulkanInstance()  = default;
    ~VulkanInstance() = default;

    VulkanInstance( const VulkanInstance& ) = delete;
    VulkanInstance& operator=( const VulkanInstance& ) = delete;

    // Crea el VkInstance y, si las validation layers están activas,
    // el debug messenger. Debe llamarse al inicio de RenderEngine::init().
    void init( const std::string& appName );

    // Destruye el debug messenger y el VkInstance.
    // Debe llamarse al final de RenderEngine::cleanup(), tras destruir
    // device y surface.
    void destroy();

    VkInstance getInstance() const { return _instance; }

    // Conveniente para pasarlo a VulkanDevice::init() y VulkanWindow::createSurface().
    operator VkInstance() const { return _instance; }

private:
    bool        checkValidationLayerSupport() const;
    std::vector<const char*> getRequiredExtensions() const;
    void        setupDebugMessenger();
    void        populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& info ) const;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT   severity,
        VkDebugUtilsMessageTypeFlagsEXT          type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* )
    {
        if ( severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) {
            std::cerr << "validation layer: " << data->pMessage << "\n";
        }
        return VK_FALSE;
    }

    VkInstance               _instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger  = VK_NULL_HANDLE;
};
