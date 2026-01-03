#include "Mesh.h"

Mesh::Mesh( VulkanDevice& device, const std::string& path ):_device(device) {



}

Mesh::Mesh( VulkanDevice& device, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices ):_device( device ) {

	_indices = indices;
	_vertices = vertices;

	_vertexBuffer = _device.createVkBuffer<Vertex>( _vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	_indexBuffer = _device.createVkBuffer<uint32_t>( _indices,VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

Mesh::Mesh( VulkanDevice& device, const std::vector<Vertex>& vertices ) :_device(device){
	
	for (uint32_t i = 0; i < vertices.size();i++) {
		_indices.push_back( i );
	}

	_vertices = vertices;

	_vertexBuffer = _device.createVkBuffer<Vertex>( _vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	_indexBuffer = _device.createVkBuffer<uint32_t>( _indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

Mesh::~Mesh()
{
	delete _vertexBuffer;
	delete _indexBuffer;
}

void Mesh::draw( VkCommandBuffer commandBuffer )
{
	VkBuffer vertexBuffers[] = { _vertexBuffer->getBuffer()};
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers( commandBuffer, 0, 1, vertexBuffers, offsets );
	vkCmdBindIndexBuffer( commandBuffer, _indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed( commandBuffer, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0 );
}
