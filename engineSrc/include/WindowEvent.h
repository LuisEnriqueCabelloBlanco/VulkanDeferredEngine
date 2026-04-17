#pragma once

// Contrato de evento de ventana desacoplado del backend.
// La app traduce eventos nativos (SDL u otro) a esta estructura,
// y el motor (RenderEngine/VulkanWindow) la consume para actualizar su estado.
enum class WindowEventType {
	Unknown,
	Resized,
};

struct WindowEvent {
	WindowEventType type = WindowEventType::Unknown;
	int width = 0;
	int height = 0;
};
