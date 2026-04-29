#include "VulkanInstance.h"

#include <SDL2/SDL_vulkan.h>
#include <iostream>
#include <stdexcept>
#include <cstring>

// Validation layers activadas en debug y desactivadas en relase
static const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
static constexpr bool enableValidationLayers = false;
#else
static constexpr bool enableValidationLayers = true;
#endif


// ---------------------------------------------------------------------------
// Funciones de extensión — son de instancia, no existen en la tabla estática
// ---------------------------------------------------------------------------

static VkResult createDebugMessenger(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks*              pAllocator,
    VkDebugUtilsMessengerEXT*                 pMessenger )
{
    auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" ) );
    return fn ? fn( instance, pCreateInfo, pAllocator, pMessenger )
              : VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void destroyDebugMessenger(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     messenger,
    const VkAllocationCallbacks* pAllocator )
{
    auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" ) );
    if ( fn ) fn( instance, messenger, pAllocator );
}

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------

void VulkanInstance::init( const std::string& appName )
{
    if ( enableValidationLayers && !checkValidationLayerSupport() ) {
        throw std::runtime_error( "Validation layers requested but not available" );
    }

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = appName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "LL Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>( extensions.size() );
    createInfo.ppEnabledExtensionNames = extensions.data();

    // El debug messenger en pNext captura también los mensajes de
    // vkCreateInstance / vkDestroyInstance, que ocurren fuera del messenger
    // normal porque este todavía no existe.
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    if ( enableValidationLayers ) {
        createInfo.enabledLayerCount   = static_cast<uint32_t>( validationLayers.size() );
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo( debugInfo );
        createInfo.pNext = &debugInfo;
    }

    if ( vkCreateInstance( &createInfo, nullptr, &_instance ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create VkInstance" );
    }

    setupDebugMessenger();
}

void VulkanInstance::destroy()
{
    if ( _debugMessenger != VK_NULL_HANDLE ) {
        destroyDebugMessenger( _instance, _debugMessenger, nullptr );
        _debugMessenger = VK_NULL_HANDLE;
    }

    if ( _instance != VK_NULL_HANDLE ) {
        vkDestroyInstance( _instance, nullptr );
        _instance = VK_NULL_HANDLE;
    }
}

// ---------------------------------------------------------------------------
// Helpers privados
// ---------------------------------------------------------------------------

bool VulkanInstance::checkValidationLayerSupport() const
{
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties( &count, nullptr );
    std::vector<VkLayerProperties> available( count );
    vkEnumerateInstanceLayerProperties( &count, available.data() );

    for ( const char* name : validationLayers ) {
        bool found = false;
        for ( const auto& prop : available ) {
            if ( strcmp( name, prop.layerName ) == 0 ) { found = true; break; }
        }
        if ( !found ) return false;
    }
    return true;
}

std::vector<const char*> VulkanInstance::getRequiredExtensions() const
{
    uint32_t sdlCount = 0;
    if ( SDL_Vulkan_GetInstanceExtensions( nullptr, &sdlCount, nullptr ) == SDL_FALSE ) {
        throw std::runtime_error( SDL_GetError() );
    }
    std::vector<const char*> extensions( sdlCount );
    SDL_Vulkan_GetInstanceExtensions( nullptr, &sdlCount, extensions.data() );

    extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
    if ( enableValidationLayers ) {
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }
    return extensions;
}

void VulkanInstance::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& info ) const
{
    info = {};
    info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugCallback;
}

void VulkanInstance::setupDebugMessenger()
{
    if ( !enableValidationLayers ) return;

    VkDebugUtilsMessengerCreateInfoEXT info{};
    populateDebugMessengerCreateInfo( info );

    if ( createDebugMessenger( _instance, &info, nullptr, &_debugMessenger ) != VK_SUCCESS ) {
        throw std::runtime_error( "Failed to create debug messenger" );
    }
}
