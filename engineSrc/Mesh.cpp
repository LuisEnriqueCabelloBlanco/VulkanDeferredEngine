#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <cmath>
#include <iostream>
#include <stdexcept>


const Mesh* Mesh::_lastRenderedMesh = nullptr;

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
	if (_lastRenderedMesh == this) {
		_lastRenderedMesh = nullptr;
	}

	delete _vertexBuffer;
	delete _indexBuffer;
}

void Mesh::draw( VkCommandBuffer commandBuffer ) const
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
		const std::string error = reader.Error();
		throw std::runtime_error( "TinyObjReader no pudo cargar el OBJ: " + path + "\n" + error );
	}

	if (!reader.Warning().empty()) {
		std::cerr << "TinyObjReader warning (" << path << "): " << reader.Warning() << "\n";
	}

	const tinyobj::attrib_t& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	for (auto mat : materials) {
		std::cout << mat.diffuse_texname << "\n";
	}


	//Map para evitar la repeticion de vertices
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	std::vector<glm::vec3> tangentSums;


	for (const auto& shape : shapes) {
		size_t indexOffset = 0;

		for (const auto& face : shape.mesh.num_face_vertices) {

			size_t fv = size_t( face );
			std::vector<uint32_t> faceVertexIndices;
			faceVertexIndices.reserve( fv );

			for (size_t v = 0; v < fv;v++) {
				auto index = shape.mesh.indices[indexOffset+v];

				Vertex vertex{};

				if (index.vertex_index < 0 || (3 * static_cast<size_t>(index.vertex_index) + 2) >= attrib.vertices.size()) {
					throw std::runtime_error( "OBJ con indice de posicion invalido en: " + path );
				}

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = glm::vec2( 0.0f );
				if (index.texcoord_index >= 0) {
					const size_t texCoordOffset = static_cast<size_t>(index.texcoord_index) * 2;
					if (texCoordOffset + 1 < attrib.texcoords.size()) {
						vertex.texCoord = {
							attrib.texcoords[texCoordOffset + 0],
							1.0f - attrib.texcoords[texCoordOffset + 1]
						};
					}
				}

				vertex.color = glm::vec3( 1.0f );
				if (index.vertex_index >= 0) {
					const size_t colorOffset = static_cast<size_t>(index.vertex_index) * 3;
					if (colorOffset + 2 < attrib.colors.size()) {
						vertex.color = {
							attrib.colors[colorOffset + 0],
							attrib.colors[colorOffset + 1],
							attrib.colors[colorOffset + 2],
						};
					}
				}


				vertex.normal = glm::vec3( 0.0f, 1.0f, 0.0f );
				if (index.normal_index >= 0) {
					const size_t normalOffset = static_cast<size_t>(index.normal_index) * 3;
					if (normalOffset + 2 < attrib.normals.size()) {
						vertex.normal = {
							attrib.normals[normalOffset + 0],
							attrib.normals[normalOffset + 1],
							attrib.normals[normalOffset + 2],
						};
					}
				}



				//si el vetice no esta lo aniadimos a la lista de vertices unicos
				if (uniqueVertices.count( vertex ) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
					_vertices.push_back( vertex );
					tangentSums.emplace_back( 0.0f );
				}

				//aniadimos el indice correspondiente
				const uint32_t vertexIndex = uniqueVertices[vertex];
				_indices.push_back( vertexIndex );
				faceVertexIndices.push_back( vertexIndex );
			}

			if (faceVertexIndices.size() >= 3) {
				// Triangulamos caras n-gon como fan: (0, tri, tri+1).
				for (size_t tri = 1; tri + 1 < faceVertexIndices.size(); ++tri) {
					const uint32_t i0 = faceVertexIndices[0];
					const uint32_t i1 = faceVertexIndices[tri];
					const uint32_t i2 = faceVertexIndices[tri + 1];

					const glm::vec3& p0 = _vertices[i0].pos;
					const glm::vec3& p1 = _vertices[i1].pos;
					const glm::vec3& p2 = _vertices[i2].pos;

					const glm::vec2& uv0 = _vertices[i0].texCoord;
					const glm::vec2& uv1 = _vertices[i1].texCoord;
					const glm::vec2& uv2 = _vertices[i2].texCoord;

					const glm::vec3 edge1 = p1 - p0;
					const glm::vec3 edge2 = p2 - p0;
					const glm::vec2 deltaUV1 = uv1 - uv0;
					const glm::vec2 deltaUV2 = uv2 - uv0;

					// Determinante de la matriz UV: si es ~0, el mapeado es degenerado y no define tangente estable.
					const float determinant = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
					if (std::abs( determinant ) <= 1e-8f) {
						continue;
					}

					// Formula clasica de tangente en espacio objeto a partir de derivadas de posicion respecto a UV.
					const float invDeterminant = 1.0f / determinant;
					const glm::vec3 tangent = invDeterminant * (edge1 * deltaUV2.y - edge2 * deltaUV1.y);
					if (glm::dot( tangent, tangent ) <= 1e-12f) {
						continue;
					}

					// Acumulamos por vertice para suavizar entre triangulos adyacentes.
					tangentSums[i0] += tangent;
					tangentSums[i1] += tangent;
					tangentSums[i2] += tangent;
				}
			}

			indexOffset += fv;
		}
	}

	for (size_t i = 0; i < _vertices.size(); ++i) {
		// Normalizamos la tangente acumulada final por vertice.
		if (glm::dot( tangentSums[i], tangentSums[i] ) > 1e-12f) {
			_vertices[i].tangent = glm::normalize( tangentSums[i] );
			continue;
		}

		// Fallback cuando no hay UV utiles: construimos una tangente ortogonal a la normal.
		const glm::vec3& normal = _vertices[i].normal;
		const glm::vec3 reference = std::abs( normal.y ) < 0.999f ? glm::vec3( 0.0f, 1.0f, 0.0f ) : glm::vec3( 1.0f, 0.0f, 0.0f );
		glm::vec3 fallbackTangent = glm::cross( reference, normal );
		if (glm::dot( fallbackTangent, fallbackTangent ) <= 1e-12f) {
			fallbackTangent = glm::vec3( 1.0f, 0.0f, 0.0f );
		}
		_vertices[i].tangent = glm::normalize( fallbackTangent );
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
