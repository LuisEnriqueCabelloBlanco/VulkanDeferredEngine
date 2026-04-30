#include "CullManager.h"

#include <chrono>
#include <iostream>

std::vector<std::vector<int>> CullManager::cullObjects(
    const std::vector<RenderObject>&       objects,
    const std::vector<ViewProjectionData>& vps,
    const ResourceManager&                 resources ) const
{
    const size_t frustumCount = vps.size();

    // Precalculamos las matrices VP de cada frustum para no repetir la
    // multiplicación en el loop interno.
    std::vector<glm::mat4> vpMats;
    vpMats.reserve( frustumCount );
    for ( const auto& vp : vps ) {
        vpMats.push_back( vp.proj * vp.view );
    }

    // Resultado: un vector de índices por frustum, preallocado.
    std::vector<std::vector<int>> results( frustumCount );
    for ( auto& r : results ) {
        r.reserve( objects.size() );
    }

    // auto start = std::chrono::high_resolution_clock::now();

    for ( int i = 0; i < static_cast<int>( objects.size() ); ++i ) {
        const Mesh* mesh = resources.tryGetMesh( objects[i].mesh );
        if ( mesh == nullptr ) {
            continue;
        }

        const AABB& aabb = mesh->getAABB();

        for ( size_t f = 0; f < frustumCount; ++f ) {
            glm::mat4 MVP = vpMats[f] * objects[i].modelMatrix;
            if ( AABBFrustumTest( aabb, MVP ) ) {
                results[f].push_back( i );
            }
        }
    }

    /* DEBUG
    auto end = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration<float, std::chrono::milliseconds::period>( end - start ).count();
    std::cout << "CullManager: "
              << frustumCount << " frustum(s), "
              << objects.size() << " objects processed in "
              << elapsed << " ms\n";
    */

    return results;
}

bool CullManager::AABBFrustumTest( const AABB& aabb, const glm::mat4& MVP ) const
{
    const glm::vec4 corners[8] = {
        { aabb.min.x, aabb.min.y, aabb.min.z, 1.f },
        { aabb.max.x, aabb.min.y, aabb.min.z, 1.f },
        { aabb.min.x, aabb.max.y, aabb.min.z, 1.f },
        { aabb.max.x, aabb.max.y, aabb.min.z, 1.f },
        { aabb.min.x, aabb.min.y, aabb.max.z, 1.f },
        { aabb.max.x, aabb.min.y, aabb.max.z, 1.f },
        { aabb.min.x, aabb.max.y, aabb.max.z, 1.f },
        { aabb.max.x, aabb.max.y, aabb.max.z, 1.f },
    };

    for ( const auto& c : corners ) {
        const glm::vec4 clip = MVP * c;
        if ( clip.x <= clip.w && clip.x > -clip.w &&
             clip.y <= clip.w && clip.y > -clip.w &&
             clip.z <= clip.w && clip.w > 0.f ) {
            return true;
        }
    }
    return false;
}
