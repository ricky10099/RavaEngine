#include "ravapch.h"

#include <ufbx/ufbx.c>

#include "Framework/RavaUtils.h"
#include "Framework/Resources/ufbxLoader.h"
#include "Framework/Resources/Materials.h"
#include "Framework/Vulkan/MaterialDescriptor.h"
#include "Framework/Vulkan/Descriptor.h"
#include "Framework/Vulkan/Renderer.h"

extern std::shared_ptr<Vulkan::Buffer> g_DummyBuffer;

namespace Rava {
ufbxLoader::ufbxLoader(const std::string& filePath)
	: m_filePath(filePath) {
	m_path = GetPathWithoutFileName(filePath);
}

bool ufbxLoader::LoadModel(const u32 instanceCount) {
	ufbx_load_opts loadOptions{};
	loadOptions.ignore_animation              = true;
	loadOptions.load_external_files           = true;
	loadOptions.ignore_missing_external_files = true;
	loadOptions.generate_missing_normals      = true;
	loadOptions.target_axes                   = ufbx_axes_left_handed_z_up;
		/*{
						  .right = UFBX_COORDINATE_AXIS_POSITIVE_X,
						  .up    = UFBX_COORDINATE_AXIS_POSITIVE_Y,
						  .front = UFBX_COORDINATE_AXIS_POSITIVE_Z,
    };*/
	loadOptions.target_unit_meters = 1.0f;

	// load raw data of the file (can be fbx or obj)
	ufbx_error ufbxError;

	m_modelScene = ufbx_load_file(m_filePath.data(), &loadOptions, &ufbxError);

	if (m_modelScene == nullptr) {
		char errorBuffer[512];
		ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
		ENGINE_ERROR("ufbxLoader::Load error: file: {0}, error: {1}", m_filePath, errorBuffer);
		return false;
	}

	if (!m_modelScene->meshes.count) {
		ENGINE_ERROR("ufbxBuilder::Load: no meshes found in {0}", m_filePath);
		return false;
	}

	LoadSkeletons();
	LoadMaterials();

	m_instanceCount = instanceCount;
	for (m_instanceIndex = 0; m_instanceIndex < m_instanceCount; ++m_instanceIndex) {
		LoadNode(m_modelScene->root_node);
	}

	ufbx_free_scene(m_modelScene);
	return true;
}

void ufbxLoader::LoadMaterials() {
	u32 numMaterials = static_cast<u32>(m_modelScene->materials.count);
	materials.resize(numMaterials);
	// m_materialTextures.resize(numMaterials);
	for (u32 materialIndex = 0; materialIndex < numMaterials; ++materialIndex) {
		const ufbx_material* fbxMaterial = m_modelScene->materials[materialIndex];
		// PrintProperties(fbxMaterial);

		LoadMaterial(fbxMaterial, UFBX_MATERIAL_PBR_BASE_COLOR, materialIndex);
		LoadMaterial(fbxMaterial, UFBX_MATERIAL_PBR_ROUGHNESS, materialIndex);
		LoadMaterial(fbxMaterial, UFBX_MATERIAL_PBR_METALNESS, materialIndex);
		LoadMaterial(fbxMaterial, UFBX_MATERIAL_PBR_NORMAL_MAP, materialIndex);
		LoadMaterial(fbxMaterial, UFBX_MATERIAL_PBR_EMISSION_COLOR, materialIndex);
		LoadMaterial(fbxMaterial, UFBX_MATERIAL_PBR_EMISSION_FACTOR, materialIndex);

		m_materialNameToIndex[fbxMaterial->name.data] = materialIndex;
	}
}

void ufbxLoader::LoadMaterial(const ufbx_material* fbxMaterial, ufbx_material_pbr_map materialProperty, int materialIndex) {
	Material& material                           = materials[materialIndex];
	Material::PBRMaterial& pbrMaterial           = material.pbrMaterial;
	Material::MaterialTextures& materialTextures = material.materialTextures;

	switch (materialProperty) {
		// aka albedo aka diffuse color
		case UFBX_MATERIAL_PBR_BASE_COLOR: {
			ufbx_material_map const& materialMap = fbxMaterial->pbr.base_color;
			if (materialMap.has_value) {
				const ufbx_material_map& baseFactorMaterialMap = fbxMaterial->pbr.base_factor;
				float baseFactor = baseFactorMaterialMap.has_value ? (float)baseFactorMaterialMap.value_real : 1.0f;
				if (materialMap.texture) {
					if (auto texture = LoadTexture(materialMap, Texture::USE_SRGB)) {
						materialTextures[Material::DIFFUSE_MAP_INDEX] = texture;
						pbrMaterial.features |= Material::HAS_DIFFUSE_MAP;
						pbrMaterial.diffuseColor.r = baseFactor;
						pbrMaterial.diffuseColor.g = baseFactor;
						pbrMaterial.diffuseColor.b = baseFactor;
						pbrMaterial.diffuseColor.a = baseFactor;
					}
				} else {
					pbrMaterial.diffuseColor.r = (float)materialMap.value_vec4.x * baseFactor;
					pbrMaterial.diffuseColor.g = (float)materialMap.value_vec4.y * baseFactor;
					pbrMaterial.diffuseColor.b = (float)materialMap.value_vec4.z * baseFactor;
					pbrMaterial.diffuseColor.a = (float)materialMap.value_vec4.w * baseFactor;
				}
			}
			break;
		}
		case UFBX_MATERIAL_PBR_ROUGHNESS: {
			ufbx_material_map const& materialMap = fbxMaterial->pbr.roughness;
			if (materialMap.has_value) {
				if (materialMap.texture) {
					if (auto texture = LoadTexture(materialMap, Texture::USE_UNORM)) {
						materialTextures[Material::ROUGHNESS_MAP_INDEX] = texture;
						pbrMaterial.features |= Material::HAS_ROUGHNESS_MAP;
					}
				} else  // constant material property
				{
					pbrMaterial.roughness = (float)materialMap.value_real;
				}
			}
			break;
		}
		case UFBX_MATERIAL_PBR_METALNESS: {
			ufbx_material_map const& materialMap = fbxMaterial->pbr.metalness;
			if (materialMap.has_value) {
				if (materialMap.texture) {
					if (auto texture = LoadTexture(materialMap, Texture::USE_UNORM)) {
						materialTextures[Material::METALLIC_MAP_INDEX] = texture;
						pbrMaterial.features |= Material::HAS_METALLIC_MAP;
					}
				} else  // constant material property
				{
					pbrMaterial.metallic = (float)materialMap.value_real;
				}
			}
			break;
		}
		case UFBX_MATERIAL_PBR_NORMAL_MAP: {
			ufbx_material_map const& materialMap = fbxMaterial->pbr.normal_map;
			if (materialMap.texture) {
				if (auto texture = LoadTexture(materialMap, Texture::USE_UNORM)) {
					materialTextures[Material::NORMAL_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_NORMAL_MAP;
				}
			}
			break;
		}
		case UFBX_MATERIAL_PBR_EMISSION_COLOR: {
			ufbx_material_map const& materialMap = fbxMaterial->pbr.emission_color;
			if (materialMap.texture) {
				if (auto texture = LoadTexture(materialMap, Texture::USE_SRGB)) {
					materialTextures[Material::EMISSIVE_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_EMISSIVE_MAP;
					pbrMaterial.emissiveColor = glm::vec3(1.0f);
				}
			} else {
				glm::vec3 emissiveColor(materialMap.value_vec3.x, materialMap.value_vec3.y, materialMap.value_vec3.z);
				pbrMaterial.emissiveColor = emissiveColor;
			}
			break;
		}
		case UFBX_MATERIAL_PBR_EMISSION_FACTOR: {
			ufbx_material_map const& materialMap = fbxMaterial->pbr.emission_factor;
			if (materialMap.has_value) {
				pbrMaterial.emissiveStrength = (float)materialMap.value_real;
			}
			break;
		}
		default: {
			ENGINE_ASSERT(false, "Material Property not recognized");
			break;
		}
	}
}

std::shared_ptr<Texture> Rava::ufbxLoader::LoadTexture(ufbx_material_map const& materialMap, bool useSRGB) {
	std::shared_ptr<Texture> texture;
	auto createTexture = [&](ufbx_string const& str) {
		std::string filepath(str.data);
		if (FileExists(filepath) && !IsDirectory(filepath)) {
			texture = std::make_shared<Texture>();
			if (texture->Init(filepath, useSRGB)) {
				// m_textures.push_back(texture);
				return true;
			}
		}
		return false;
	};

	if (createTexture(materialMap.texture->filename)) {
		return texture;
	}
	if (createTexture(materialMap.texture->absolute_filename)) {
		return texture;
	}
	if (createTexture(materialMap.texture->relative_filename)) {
		return texture;
	}
	if (!texture) {
		std::string textureName = materialMap.texture->filename.data;
		textureName             = textureName.substr(textureName.find_last_of("\\") + 1);
		// m_filePath;
		std::string texturepath(GetPathWithoutFileName(m_filePath) + textureName);
		texture = std::make_shared<Texture>();
		if (texture->Init(texturepath, useSRGB)) {
			// m_textures.push_back(texture);
			return texture;
		}
	}

	std::string filepath(materialMap.texture->filename.data);
	ENGINE_ERROR("ufbxLoader::LoadTexture(): file '{0}' not found", filepath);
	return nullptr;
}

void ufbxLoader::LoadNode(const ufbx_node* fbxNode) {
	//ufbx_matrix transform = ufbx_matrix_mul(&parentTransform, &fbxNode->node_to_parent);

	//nodes.push_back(Node{});
	//nodes.back().parentNode = parentNode;
	//nodes.back().transform  = ufbxToglm(fbxNode->node_to_parent);

 //   i32 thisID                          = (i32)nodes.size() - 1;
	//m_nodeMap[fbxNode->name.data] = thisID;
	//if (parentNode >= 0) {
	//	nodes[parentNode].children.push_back(thisID);
	//}

	ufbx_mesh* fbxMesh = fbxNode->mesh;
	if (fbxMesh) {
		u32 meshCount = static_cast<u32>(fbxMesh->material_parts.count);
		if (meshCount > 0) {
			vertices.clear();
			indices.clear();
			meshes.clear();

			meshes.resize(meshCount);
			for (u32 meshIndex = 0; meshIndex < meshCount; ++meshIndex) {
				LoadMesh(fbxNode, meshIndex);
				std::string materialName = fbxNode->mesh->materials.data[meshIndex]->name.data;
				u32 materialIndex        = m_materialNameToIndex[materialName];
				AssignMaterial(meshes[meshIndex], materialIndex);
			}
			if (m_fbxNoTangents)  // at least one mesh did not have tangents
			{
				CalculateTangents();
			}
		}
	}
	u32 childCount = static_cast<u32>(fbxNode->children.count);
	for (u32 childIndex = 0; childIndex < childCount; ++childIndex) {
		LoadNode(fbxNode->children[childIndex]);
	}
}

void ufbxLoader::LoadMesh(const ufbx_node* fbxNode, const u32 meshIndex) {
	ufbx_mesh* fbxMesh                = fbxNode->mesh;
	const ufbx_mesh_part& fbxMeshPart = fbxNode->mesh->material_parts[meshIndex];
	size_t faceCount                  = fbxMeshPart.num_faces;

	if (!fbxMeshPart.num_triangles) {
		ENGINE_ERROR("ufbxLoader::LoadMesh: only triangle meshes are supported!");
		return;
	}

	size_t numVerticesBefore = vertices.size();
	size_t numIndicesBefore  = indices.size();

	Mesh& mesh         = meshes[meshIndex];
	mesh.firstVertex   = static_cast<u32>(numVerticesBefore);
	mesh.firstIndex    = static_cast<u32>(numIndicesBefore);
	mesh.indexCount    = 0;
	mesh.instanceCount = m_instanceCount;

	ufbx_material_map& baseColorMap = fbxNode->materials[meshIndex]->pbr.base_color;
	glm::vec4 diffuseColor =
		baseColorMap.has_value ? glm::vec4(
			baseColorMap.value_vec4.x, baseColorMap.value_vec4.y, baseColorMap.value_vec4.z, baseColorMap.value_vec4.w
		)
							   : glm::vec4(1.0f);

#pragma region Vertices
	bool hasTangents            = fbxMesh->vertex_tangent.exists;
	bool hasUVs                 = fbxMesh->uv_sets.count;
	bool hasVertexColors        = fbxMesh->vertex_color.exists;
	ufbx_skin_deformer* fbxSkin = nullptr;
	if (fbxMesh->skin_deformers.count) {
		fbxSkin = fbxMesh->skin_deformers.data[0];
	}

	m_fbxNoTangents = m_fbxNoTangents || (!hasTangents);
	for (size_t fbxFaceIndex = 0; fbxFaceIndex < faceCount; ++fbxFaceIndex) {
		ufbx_face& fbxFace        = fbxMesh->faces[fbxMeshPart.face_indices.data[fbxFaceIndex]];
		size_t triangleIndexCount = fbxMesh->max_face_triangles * 3;
		std::vector<u32> verticesPerFaceIndexBuffer(triangleIndexCount);
		size_t triangleCount = ufbx_triangulate_face(verticesPerFaceIndexBuffer.data(), triangleIndexCount, fbxMesh, fbxFace);
		size_t vertexCountPerFace = triangleCount * 3;

		for (u32 vertexPerFace = 0; vertexPerFace < vertexCountPerFace; ++vertexPerFace) {
			u32 vertexPerFaceIndex = verticesPerFaceIndexBuffer[vertexPerFace];

			Vertex vertex{};

			u32 fbxVertexIndex     = fbxMesh->vertex_indices[vertexPerFaceIndex];
			ufbx_vec3& positionFbx = fbxMesh->vertices[fbxVertexIndex];
			vertex.position        = glm::vec3(positionFbx.x, positionFbx.y, positionFbx.z);

			u32 fbxNormalIndex = fbxMesh->vertex_normal.indices[vertexPerFaceIndex];
			ENGINE_ASSERT(fbxNormalIndex < fbxMesh->vertex_normal.values.count, "LoadMesh: memory violation normals");
			ufbx_vec3& normalFbx = fbxMesh->vertex_normal.values.data[fbxNormalIndex];
			vertex.normal        = glm::vec3(normalFbx.x, normalFbx.y, normalFbx.z);

			if (hasTangents) {
				u32 fbxTangentIndex = fbxMesh->vertex_tangent.indices[vertexPerFaceIndex];
				ENGINE_ASSERT(fbxTangentIndex < fbxMesh->vertex_tangent.values.count, "LoadMesh: memory violation tangents");
				ufbx_vec3& tangentFbx = fbxMesh->vertex_tangent.values.data[fbxTangentIndex];
				vertex.tangent        = glm::vec3(tangentFbx.x, tangentFbx.y, tangentFbx.z);
			}

			if (hasUVs) {
				u32 fbxUVIndex = fbxMesh->vertex_uv.indices[vertexPerFaceIndex];
				ENGINE_ASSERT(fbxUVIndex < fbxMesh->vertex_uv.values.count, "LoadMesh: memory violation uv coordinates");
				ufbx_vec2& uvFbx = fbxMesh->vertex_uv.values.data[fbxUVIndex];
				vertex.uv        = glm::vec2(uvFbx.x, uvFbx.y);
			}

			if (hasVertexColors) {
				u32 fbxColorIndex   = fbxMesh->vertex_color.indices[vertexPerFaceIndex];
				ufbx_vec4& colorFbx = fbxMesh->vertex_color.values.data[fbxColorIndex];

				// convert from sRGB to linear
				glm::vec3 linearColor = glm::pow(glm::vec3(colorFbx.x, colorFbx.y, colorFbx.z), glm::vec3(2.2f));
				glm::vec4 vertexColor(linearColor.x, linearColor.y, linearColor.z, colorFbx.w);
				vertex.color = vertexColor * diffuseColor;
			} else {
				vertex.color = diffuseColor;
			}

			if (fbxSkin) {
				ufbx_skin_vertex skinVertex = fbxSkin->vertices[fbxVertexIndex];
				size_t numWeights = skinVertex.num_weights < MAX_JOINT_INFLUENCE ? skinVertex.num_weights : MAX_JOINT_INFLUENCE;

				for (size_t weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
					ufbx_skin_weight skinWeight = fbxSkin->weights.data[skinVertex.weight_begin + weightIndex];
					int jointIndex              = skinWeight.cluster_index;
					float weight                = (float)skinWeight.weight;

					switch (weightIndex) {
						case 0:
							vertex.jointIds.x = jointIndex;
							vertex.weights.x  = weight;
							break;
						case 1:
							vertex.jointIds.y = jointIndex;
							vertex.weights.y  = weight;
							break;
						case 2:
							vertex.jointIds.z = jointIndex;
							vertex.weights.z  = weight;
							break;
						case 3:
							vertex.jointIds.w = jointIndex;
							vertex.weights.w  = weight;
							break;
						default:
							break;
					}
				}

				// normalize weights
				float weightSum = vertex.weights.x + vertex.weights.y + vertex.weights.z + vertex.weights.w;
				if (weightSum > std::numeric_limits<float>::epsilon()) {
					vertex.weights = vertex.weights / weightSum;
				}
			}

			vertices.push_back(vertex);
		}
	}
#pragma endregion

#pragma region Indices
	u32 meshAllVertices = static_cast<u32>(vertices.size() - numVerticesBefore);

	ufbx_vertex_stream stream;
	stream.data         = &vertices[numVerticesBefore];
	stream.vertex_count = meshAllVertices;
	stream.vertex_size  = sizeof(Vertex);

	indices.resize(numIndicesBefore + meshAllVertices);

	ufbx_error ufbxError;
	size_t vertexCount = ufbx_generate_indices(&stream, 1, &indices[numIndicesBefore], meshAllVertices, nullptr, &ufbxError);

	if (ufbxError.type != UFBX_ERROR_NONE) {
		char errorBuffer[512];
		ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
		ENGINE_ERROR(
			"ufbxBuilder: creation of index buffer failed, file: {0}, error: {1},  node: {2}",
			m_filePath,
			errorBuffer,
			fbxNode->name.data
		);
	}

	vertices.resize(numVerticesBefore + vertexCount);
	mesh.vertexCount = static_cast<u32>(vertexCount);
	mesh.indexCount  = meshAllVertices;
#pragma endregion
}

void ufbxLoader::AssignMaterial(Mesh& mesh, const int materialIndex) {
	// material
	{
		if (!(static_cast<size_t>(materialIndex) < materials.size())) {
			ENGINE_ERROR("AssignMaterial: materialIndex must be less than materials.size()");
		}

		Material& material = mesh.material;

		// material
		if (materialIndex != -1) {
			material = materials[materialIndex];
			// material.materialTextures = m_materialTextures[materialIndex];
		}

		// create material descriptor
		material.materialDescriptor = std::make_shared<Vulkan::MaterialDescriptor>(mesh.material, mesh.material.materialTextures);
	}
	ENGINE_INFO("Material assigned (ufbx): material index {0}", materialIndex);

	if (skeletonUbo) {
		mesh.skeletonBuffer = skeletonUbo;
	} else {
		mesh.skeletonBuffer = g_DummyBuffer;
	}

	Vulkan::DescriptorSetLayout::Builder builder{};
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	Unique<Vulkan::DescriptorSetLayout> localDescriptorSetLayout = builder.Build();
	Vulkan::DescriptorWriter descriptorWriter(*localDescriptorSetLayout, *Vulkan::Renderer::s_descriptorPool);
	VkDescriptorBufferInfo bufferInfo = mesh.skeletonBuffer->DescriptorInfo();
	descriptorWriter.WriteBuffer(0, &bufferInfo);
	descriptorWriter.Build(mesh.skeletonDescriptorSet);
}

void ufbxLoader::CalculateTangents() {
	if (indices.size()) {
		CalculateTangentsFromIndexBuffer(indices);
	} else {
		u32 vertexCount = static_cast<u32>(vertices.size());
		if (vertexCount) {
			std::vector<u32> indices;
			indices.resize(vertexCount);
			for (u32 i = 0; i < vertexCount; i++) {
				indices[i] = i;
			}
			CalculateTangentsFromIndexBuffer(indices);
		}
	}
}

void ufbxLoader::CalculateTangentsFromIndexBuffer(const std::vector<u32>& indices) {
	u32 count           = 0;
	u32 vertexIndex1    = 0;
	u32 vertexIndex2    = 0;
	u32 vertexIndex3    = 0;
	glm::vec3 position1 = glm::vec3{0.0f};
	glm::vec3 position2 = glm::vec3{0.0f};
	glm::vec3 position3 = glm::vec3{0.0f};
	glm::vec2 uv1       = glm::vec2{0.0f};
	glm::vec2 uv2       = glm::vec2{0.0f};
	glm::vec2 uv3       = glm::vec2{0.0f};

	for (u32 index : indices) {
		auto& vertex = vertices[index];

		switch (count) {
			case 0:
				position1    = vertex.position;
				uv1          = vertex.uv;
				vertexIndex1 = index;
				break;
			case 1:
				position2    = vertex.position;
				uv2          = vertex.uv;
				vertexIndex2 = index;
				break;
			case 2:
				position3    = vertex.position;
				uv3          = vertex.uv;
				vertexIndex3 = index;

				glm::vec3 edge1    = position2 - position1;
				glm::vec3 edge2    = position3 - position1;
				glm::vec2 deltaUV1 = uv2 - uv1;
				glm::vec2 deltaUV2 = uv3 - uv1;

				float dU1 = deltaUV1.x;
				float dU2 = deltaUV2.x;
				float dV1 = deltaUV1.y;
				float dV2 = deltaUV2.y;
				float E1x = edge1.x;
				float E2x = edge2.x;
				float E1y = edge1.y;
				float E2y = edge2.y;
				float E1z = edge1.z;
				float E2z = edge2.z;

				float factor;
				if ((dU1 * dV2 - dU2 * dV1) > std::numeric_limits<float>::epsilon()) {
					factor = 1.0f / (dU1 * dV2 - dU2 * dV1);
				} else {
					factor = 100000.0f;
				}

				glm::vec3 tangent;

				tangent.x = factor * (dV2 * E1x - dV1 * E2x);
				tangent.y = factor * (dV2 * E1y - dV1 * E2y);
				tangent.z = factor * (dV2 * E1z - dV1 * E2z);
				if (tangent.x == 0.0f && tangent.y == 0.0f && tangent.z == 0.0f) {
					tangent = glm::vec3(1.0f, 0.0f, 0.0f);
				}

				vertices[vertexIndex1].tangent = tangent;
				vertices[vertexIndex2].tangent = tangent;
				vertices[vertexIndex3].tangent = tangent;

				break;
		}
		count = (count + 1) % 3;
	}
}
}  // namespace Rava