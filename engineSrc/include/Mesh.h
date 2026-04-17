#pragma once
#include<vulkan/vulkan_core.h>
#include<vector>
#include<string>
#include"VulkanDevice.h"
#include"Vertex.h"

class RenderEngine;
class Buffer;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class Mesh {
    friend RenderEngine;
public:

    Mesh( const Mesh& ) = delete;
    Mesh& operator=( const Mesh& ) = delete;
    Mesh( VulkanDevice& device, const std::string& path);
    Mesh( VulkanDevice& device, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices);
    Mesh( VulkanDevice& device,  const std::vector<Vertex>& vertices);

    ~Mesh();

    void draw(VkCommandBuffer commnadBuffer);

    inline const AABB& getAABB() const { return _meshAABB; }
    //TODO loadMesh metodo estatico

private:

    static Mesh* _lastRenderedMesh;

    void loadMesh(const std::string& path);

    AABB calculateAABB();

    VulkanDevice& _device;

    AABB _meshAABB;

    std::vector<uint32_t> _indices;
    Buffer* _indexBuffer;
    std::vector<Vertex> _vertices;
    Buffer* _vertexBuffer;
};
