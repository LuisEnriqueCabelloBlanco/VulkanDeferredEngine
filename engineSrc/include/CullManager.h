#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "BufferObjectsData.h"  // ViewProjectionData
#include "Scene.h"              // RenderObject
#include "ResourceManager.h"    // ResourceManager
#include "Mesh.h"               // AABB

// ---------------------------------------------------------------------------
// CullManager
// ---------------------------------------------------------------------------
/*
  Culling CPU por frustum para objetos de la escena.

  La operación central es cullObjects(): recibe una lista de objetos y una
  lista de ViewProjectionData (uno por frustum), y devuelve en un único
  recorrido los índices visibles para cada frustum.

  El índice i del vector de salida corresponde siempre al frustum vps[i].

  Sin estado propio. Sin dependencias de Vulkan. Sin ciclo de vida.
  RenderEngine lo posee como miembro y lo llama pasando los datos exactos.
*/
class CullManager {
public:
    // Ejecuta el culling de todos los frustums en un único recorrido.
    //
    // objects   — lista completa de objetos de la escena.
    // vps       — un VP por frustum (cámara, main light, etc.).
    // resources — necesario para resolver el mesh handle de cada objeto.
    //
    // Devuelve un vector del mismo tamaño que vps. El elemento i contiene
    // los índices de los objetos visibles desde el frustum vps[i].
    std::vector<std::vector<int>> cullObjects(
        const std::vector<RenderObject>&    objects,
        const std::vector<ViewProjectionData>& vps,
        const ResourceManager&              resources ) const;

private:
    // Test AABB contra el frustum definido por MVP.
    // Devuelve true si alguna esquina de la AABB está dentro del clip space.
    bool AABBFrustumTest( const AABB& aabb, const glm::mat4& MVP ) const;
};
