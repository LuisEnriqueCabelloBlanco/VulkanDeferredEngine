#pragma once

/**
 * @file ResourceLimits.h
 * @brief Constantes de capacidad maxima de los subsistemas del motor.
 *
 * Estos valores definen los topes duros de cada registro interno. Superarlos
 * produce una excepcion del tipo correspondiente (SceneException o
 * ResourceException con codigo LimitExceeded).
 *
 * Son constantes de compilacion: cambiarlas requiere recompilar el motor.
 */

 /**
  * @brief Espacio de nombres que agrupa los limites de capacidad del motor.
  */
namespace ResourceLimits {

	/// @brief Numero maximo de frames que pueden estar en vuelo simultaneamente en la GPU.
	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	/// @brief Numero maximo de objetos que el sistema de culling puede procesar por frame.
	constexpr int MAX_CULL_OBJECTS = 100000;

	/// @brief Numero maximo de texturas que pueden existir simultaneamente en el ResourceManager.
	constexpr int MAX_TEXTURES = 32;

	/// @brief Numero maximo de luces que pueden existir simultaneamente en la Scene.
	constexpr int MAX_LIGHTS = 16384;

	/// @brief Numero maximo de entidades renderizables que pueden existir simultaneamente en la Scene.
	constexpr int MAX_ENTITIES = 16384;

	/// @brief Numero maximo de mallas que pueden existir simultaneamente en el ResourceManager.
	constexpr int MAX_MESHES = 4096;

	/// @brief Numero maximo de materiales que pueden existir simultaneamente en el ResourceManager.
	constexpr int MAX_MATERIALS = 20000;

} // namespace ResourceLimits