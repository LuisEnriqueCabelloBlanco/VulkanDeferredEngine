#pragma once
#include<vulkan/vulkan.hpp>


//TODO convertir en atributo de la clase y solicitar en la inicializacion del mismo
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME
};

#pragma region DebugMessenger

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData ) {


	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		// Message is important enough to show
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl << std::endl;
	}

	return VK_FALSE;
}

vk::DebugUtilsMessengerEXT make_debug_messenger( vk::Instance& instance, vk::DispatchLoaderDynamic& dldi ) {

	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT(
		vk::DebugUtilsMessengerCreateFlagsEXT(),
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		debugCallback,
		nullptr
	);

	return instance.createDebugUtilsMessengerEXT( debugCreateInfo, nullptr, dldi );
}
#pragma endregion

//Funcion que determina si un dispositivo fisico es adecuado para la aplicacion (TDB)
bool isDeviceSuitable(const vk::PhysicalDevice& device) {
	auto deviceProperties = device.getProperties();
	auto deviceFeatures = device.getFeatures();

	return deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && deviceFeatures.geometryShader;
}

void pickPhysicalDevice( vk::Instance& instance, vk::PhysicalDevice& physicalDevice) {
	auto devices = instance.enumeratePhysicalDevices();

	if (devices.empty()) {
		throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
	}

	for (const auto& device : devices) {
		if (isDeviceSuitable( device )) {
			physicalDevice = device;
			break;
		}
	}
}

//devuelve el indice de la queue que cumple las flags
uint32_t findQueueFamilies(const vk::PhysicalDevice& device , vk::Flags<vk::QueueFlagBits> flags) {
	std::vector < vk::QueueFamilyProperties> queueFamilyProperties = device.getQueueFamilyProperties();

	auto graphicsQueueFamilyProperty = std::find_if( queueFamilyProperties.begin(), queueFamilyProperties.end(),
													 [flags]( vk::QueueFamilyProperties const& qfp ) {return qfp.queueFlags & flags;} );

	return static_cast<uint32_t>(std::distance( queueFamilyProperties.begin(), graphicsQueueFamilyProperty ));
}

vk::Device createLogicalDevice(const vk::PhysicalDevice& device, const std::vector<const char*>& deviceExtensions) {
	vk::DeviceCreateInfo inf;

	vk::PhysicalDeviceFeatures features = {};
	device.getFeatures(&features);
	inf.setPEnabledFeatures(&features);
	inf.setPpEnabledExtensionNames( deviceExtensions.data());
	inf.setEnabledExtensionCount( deviceExtensions.size() );

	std::vector< vk::DeviceQueueCreateInfo> queues;

	float priorities = 1.0f;
	//establece que colas tendra el dispositivo
	vk::Flags<vk::QueueFlagBits> flags = vk::QueueFlagBits::eGraphics & vk::QueueFlagBits::eTransfer;
	vk::DeviceQueueCreateInfo graphicsQueueInfo;
	graphicsQueueInfo.setQueueFamilyIndex( findQueueFamilies( device,  flags));
	graphicsQueueInfo.setQueueCount( 1 );
	graphicsQueueInfo.setPQueuePriorities( &priorities );
	queues.push_back( graphicsQueueInfo );

	inf.setPQueueCreateInfos(queues.data());
	inf.setQueueCreateInfoCount( queues.size() );
	
	vk::Device logicDevice = device.createDevice(inf);
}

vk::ShaderModule createShaderModule( const std::vector<char>& code ) {
	vk::ShaderModuleCreateInfo moduleInfo;

	const uint32_t* cod = reinterpret_cast<const uint32_t*> (code.data());
	moduleInfo.setCodeSize( code.size() );
	moduleInfo.setPCode(cod);

	return vDevice.createShaderModule( moduleInfo );
}