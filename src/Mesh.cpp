#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Mesh::Mesh( VulkanDevice& device, const std::string& path ) :_device( device ) {

	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig reader_config;
	reader_config.vertex_color = true;

	if (!reader.ParseFromFile( path, reader_config )) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
	}

	const tinyobj::attrib_t& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();


	//Map para evitar la repeticion de vertices
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};


	for (const auto& shape : shapes) {

		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
			attrib.vertices[3 * index.vertex_index + 0],
			attrib.vertices[3 * index.vertex_index + 1],
			attrib.vertices[3 * index.vertex_index + 2]
			};
			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = {
				attrib.colors[3 * index.vertex_index + 0],
				attrib.colors[3 * index.vertex_index + 1],
				attrib.colors[3 * index.vertex_index + 2],
			};

			vertex.normal = {
				attrib.normals[3 * index.normal_index+ 0],
				attrib.normals[3 * index.normal_index+ 1],
				attrib.normals[3 * index.normal_index+ 2],
			};

			

			//si el vetice no esta lo aniadimos a la lista de vertices unicos
			if (uniqueVertices.count( vertex ) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
				_vertices.push_back( vertex );
			}
			//aniadimos el indice correspondiente
			_indices.push_back( uniqueVertices[vertex] );
		}
	}


	_vertexBuffer = _device.createVkBuffer<Vertex>( _vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	_indexBuffer = _device.createVkBuffer<uint32_t>( _indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

Mesh::Mesh( VulkanDevice& device, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices ) :_device( device ) {

	_indices = indices;
	_vertices = vertices;

	_vertexBuffer = _device.createVkBuffer<Vertex>( _vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	_indexBuffer = _device.createVkBuffer<uint32_t>( _indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

Mesh::Mesh( VulkanDevice& device, const std::vector<Vertex>& vertices ) :_device( device ) {

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
	VkBuffer vertexBuffers[] = { _vertexBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers( commandBuffer, 0, 1, vertexBuffers, offsets );
	vkCmdBindIndexBuffer( commandBuffer, _indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32 );
	vkCmdDrawIndexed( commandBuffer, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0 );
}
