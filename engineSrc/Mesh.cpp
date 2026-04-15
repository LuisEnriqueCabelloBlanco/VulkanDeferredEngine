#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


Mesh* Mesh::_lastRenderedMesh = nullptr;

Mesh::Mesh( Mesh& otherMesh ) :_device( otherMesh._device )
{
	_indexBuffer = otherMesh._indexBuffer;
	_vertexBuffer = otherMesh._vertexBuffer;
	_indices = otherMesh._indices;
	_vertices = otherMesh._vertices;
	_meshAABB = otherMesh._meshAABB;
	_isCopy = true;
}

Mesh::Mesh( VulkanDevice& device, const std::string& path ) :_device( device ) {

	loadMesh( path );
	
	_meshAABB = calculateAABB();

	_vertexBuffer = _device.createVkBuffer<Vertex>( _vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	_indexBuffer = _device.createVkBuffer<uint32_t>( _indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

Mesh::Mesh( VulkanDevice& device, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices ) :_device( device ) {

	_indices = indices;
	_vertices = vertices;
	_meshAABB = calculateAABB();

	_vertexBuffer = _device.createVkBuffer<Vertex>( _vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	_indexBuffer = _device.createVkBuffer<uint32_t>( _indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

Mesh::Mesh( VulkanDevice& device, const std::vector<Vertex>& vertices ) :_device( device ) {

	for (uint32_t i = 0; i < vertices.size();i++) {
		_indices.push_back( i );
	}

	_vertices = vertices;

	_meshAABB = calculateAABB();

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
	if (_lastRenderedMesh != this) {
		VkBuffer vertexBuffers[] = { _vertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers( commandBuffer, 0, 1, vertexBuffers, offsets );
		vkCmdBindIndexBuffer( commandBuffer, _indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32 );
	}
	vkCmdDrawIndexed( commandBuffer, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0 );
	
	_lastRenderedMesh = this;
}

void Mesh::loadMesh( const std::string& path )
{
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

	for (auto mat : materials) {
		std::cout << mat.diffuse_texname << "\n";
	}


	//Map para evitar la repeticion de vertices
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};


	for (const auto& shape : shapes) {
		int indexOffset = 0;

		for (const auto& face : shape.mesh.num_face_vertices) {

			size_t fv = size_t( face );

			for (int v = 0; v < fv;v++) {
				auto index = shape.mesh.indices[indexOffset+v];

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
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};

			

				//si el vetice no esta lo aniadimos a la lista de vertices unicos
				if (uniqueVertices.count( vertex ) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
					_vertices.push_back( vertex );
				}

				//aniadimos el indice correspondiente
				_indices.push_back( uniqueVertices[vertex] );
			}

			indexOffset += fv;

			//calcular tangente TODO
			glm::vec3 tangent = _vertices[_indices[_indices.size() - 1]].pos - _vertices[_indices[_indices.size() - 2]].pos;


			for (int i = 0; i < fv; i++) {
				_vertices[_indices[_indices.size() - 1 - i]].tangent = tangent;
			}

		}
	}
}

AABB Mesh::calculateAABB()
{
	AABB out{};

	out.max = glm::vec3(std::numeric_limits<float>::min());
	out.min = glm::vec3( std::numeric_limits<float>::max() );

	for (auto& pos : _vertices) {
		out.min = glm::min(pos.pos,out.min);
		out.max = glm::max(pos.pos,out.max);
	}

	return out;
}
