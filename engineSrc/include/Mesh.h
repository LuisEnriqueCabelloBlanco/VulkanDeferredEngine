#pragma once
#include<vulkan/vulkan_core.h>
#include<vector>
#include<string>
#include"BufferObjectsData.h"
#include"VulkanDevice.h"

class Buffer;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class Mesh {
public:

    Mesh( Mesh& otherMesh );
    Mesh( VulkanDevice& device, const std::string& path);
    Mesh( VulkanDevice& device, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices);
    Mesh( VulkanDevice& device,  const std::vector<Vertex>& vertices);

    ~Mesh();

    void draw(VkCommandBuffer commnadBuffer);

    inline const AABB& getAABB() { return _meshAABB; }
    //TODO loadMesh metodo estatico

private:

    void loadMesh(const std::string& path);

    AABB calculateAABB();

    VulkanDevice& _device;

    AABB _meshAABB;

    bool _isCopy = false;

    std::vector<uint32_t> _indices;
    Buffer* _indexBuffer;
    std::vector<Vertex> _vertices;
    Buffer* _vertexBuffer;
};