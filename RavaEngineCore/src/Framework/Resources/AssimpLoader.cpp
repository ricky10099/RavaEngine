#include "ravapch.h"

#include "Framework/RavaUtils.h"
#include "Framework/Resources/AssimpLoader.h"
#include "Framework/Resources/Materials.h"
#include "Framework/Vulkan/MaterialDescriptor.h"
#include "Framework/Vulkan/Descriptor.h"
#include "Framework/Vulkan/Renderer.h"

extern std::shared_ptr<Vulkan::Buffer> g_DummyBuffer;

namespace Rava {
AssimpLoader::AssimpLoader(const std::string& filePath)
	: m_filePath(filePath) {
	m_path = GetPathWithoutFileName(filePath);
}

bool AssimpLoader::LoadModel(const u32 instanceCount) {
	// Import model "scene"
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		m_filePath,
		aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices
			| aiProcess_SortByPType
	);
	if (!scene) {
		ENGINE_ERROR("Failed to load model! (" + m_filePath + ")");
		return false;
	}

	//LoadSkeletons();
	LoadMaterials();

	meshes.clear();

	// Load in all our meshes
	LoadNode(scene->mRootNode);
}

void AssimpLoader::LoadMaterials() {
	u32 numMaterials = m_modelScene->mNumMaterials;
	materials.resize(numMaterials);
	// samplerDescriptorSets.resize(numMaterials);
	for (u32 materialIndex = 0; materialIndex < numMaterials; ++materialIndex) {
		const aiMaterial* fbxMaterial = m_modelScene->mMaterials[materialIndex];
		// PrintMaps(fbxMaterial);

		Material& material = materials[materialIndex];

		LoadProperties(fbxMaterial, material.pbrMaterial);
		LoadMap(fbxMaterial, aiTextureType_DIFFUSE, materialIndex);
		LoadMap(fbxMaterial, aiTextureType_NORMALS, materialIndex);
		LoadMap(fbxMaterial, aiTextureType_SHININESS, materialIndex);
		LoadMap(fbxMaterial, aiTextureType_METALNESS, materialIndex);
		LoadMap(fbxMaterial, aiTextureType_EMISSIVE, materialIndex);
	}
}

void AssimpLoader::LoadProperties(const aiMaterial* fbxMaterial, Material::PBRMaterial& pbrMaterial) {
	// diffuse
	{
		aiColor3D diffuseColor;
		if (fbxMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == aiReturn_SUCCESS) {
			pbrMaterial.diffuseColor.r = diffuseColor.r;
			pbrMaterial.diffuseColor.g = diffuseColor.g;
			pbrMaterial.diffuseColor.b = diffuseColor.b;
		}
	}

	// roughness
	{
		float roughnessFactor;
		if (fbxMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == aiReturn_SUCCESS) {
			pbrMaterial.roughness = roughnessFactor;
		} else {
			pbrMaterial.roughness = 0.1f;
		}
	}

	// metallic
	{
		float metallicFactor;
		if (fbxMaterial->Get(AI_MATKEY_REFLECTIVITY, metallicFactor) == aiReturn_SUCCESS) {
			pbrMaterial.metallic = metallicFactor;
		} else if (fbxMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == aiReturn_SUCCESS) {
			pbrMaterial.metallic = metallicFactor;
		} else {
			pbrMaterial.metallic = 0.886f;
		}
	}

	// emissive color
	{
		aiColor3D emission;
		auto result = fbxMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emission);
		if (result == aiReturn_SUCCESS) {
			pbrMaterial.emissiveColor = glm::vec3(emission.r, emission.g, emission.b);
		}
	}

	// emissive strength
	{
		float emissiveStrength;
		auto result = fbxMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength);
		if (result == aiReturn_SUCCESS) {
			pbrMaterial.emissiveStrength = emissiveStrength;
		}
	}

	pbrMaterial.normalMapIntensity = 1.0f;
}

void AssimpLoader::LoadMap(const aiMaterial* fbxMaterial, aiTextureType textureType, int materialIndex) {
	Material& material                           = materials[materialIndex];
	Material::PBRMaterial& pbrMaterial           = material.pbrMaterial;
	Material::MaterialTextures& materialTextures = material.materialTextures;

	u32 textureCount = fbxMaterial->GetTextureCount(textureType);

	aiString aiFilepath;
	//auto getTexture = fbxMaterial->GetTexture(textureType, 0 /* first map*/, &aiFilepath);
	//std::string fbxFilepath(aiFilepath.C_Str());
	//fbxFilepath = fbxFilepath.substr(fbxFilepath.find_last_of("\\") + 1);
	//std::string filepath(m_path + fbxFilepath);
	auto getTexture = fbxMaterial->GetTexture(textureType, 0 /* first map*/, &aiFilepath);
	std::string fbxFilepath(aiFilepath.C_Str());
	std::string filepath(fbxFilepath);
	if (getTexture == aiReturn_SUCCESS) {
		switch (textureType) {
			// LoadTexture is inside switch statement for sRGB and UNORM
			case aiTextureType_DIFFUSE: {
				auto texture = LoadTexture(filepath, Texture::USE_SRGB);
				if (texture) {
					// textures.push_back(texture);
					materialTextures[Material::DIFFUSE_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_DIFFUSE_MAP;
				}
				break;
			}
			case aiTextureType_NORMALS: {
				auto texture = LoadTexture(filepath, Texture::USE_UNORM);
				if (texture) {
					materialTextures[Material::NORMAL_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_NORMAL_MAP;
				}
				break;
			}
			case aiTextureType_SHININESS:  // assimp XD
			{
				auto texture = LoadTexture(filepath, Texture::USE_UNORM);
				if (texture) {
					materialTextures[Material::ROUGHNESS_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_ROUGHNESS_MAP;
				}
				break;
			}
			case aiTextureType_METALNESS: {
				auto texture = LoadTexture(filepath, Texture::USE_UNORM);
				if (texture) {
					materialTextures[Material::METALLIC_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_METALLIC_MAP;
				}
				break;
			}
			case aiTextureType_EMISSIVE: {
				auto texture = LoadTexture(filepath, Texture::USE_SRGB);
				if (texture) {
					materialTextures[Material::EMISSIVE_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_EMISSIVE_MAP;
					pbrMaterial.emissiveColor = glm::vec3(1.0f);
				}
				break;
			}
			default: {
				ENGINE_ASSERT(false, "texture type not recognized");
			}
		}
	}
}

std::shared_ptr<Texture> AssimpLoader::LoadTexture(const std::string& filepath, bool useSRGB) {
	std::shared_ptr<Texture> texture;
	bool loadSucess = false;

	std::string fileName = filepath.substr(filepath.find_last_of("\\") + 1);

	if (FileExists(filepath) && !IsDirectory(filepath)) {
		texture    = std::make_shared<Texture>();
		loadSucess = texture->Init(filepath, useSRGB);
	} else if (FileExists(m_path + filepath) && !IsDirectory(m_path + filepath)) {
		texture    = std::make_shared<Texture>();
		loadSucess = texture->Init(m_path + filepath, useSRGB);
	} else if (FileExists(m_path + fileName) && !IsDirectory(m_path + fileName)) {
		texture    = std::make_shared<Texture>();
		loadSucess = texture->Init(m_path + fileName, useSRGB);
	} else {
		ENGINE_ERROR("AssimpLoader::LoadTexture(): file '{0}' not found", filepath);
	}

	if (loadSucess) {
		//m_textures.push_back(texture);
		return texture;
	}
	return nullptr;
}

void AssimpLoader::LoadNode(aiNode* node) {
	u32 numMeshes = node->mNumMeshes;

	u32 numMeshesBefore = static_cast<u32>(meshes.size());
	meshes.resize(numMeshes + meshes.size());

	m_fbxNoTangents = false;

	// Go through each mesh at this node and create it, then add it to our meshList
	for (u32 meshIndex = 0; meshIndex < numMeshes; ++meshIndex) {
		LoadMesh(m_modelScene->mMeshes[node->mMeshes[meshIndex]], numMeshesBefore + meshIndex);
		AssignMaterial(meshes[meshIndex], m_modelScene->mMeshes[node->mMeshes[meshIndex]]->mMaterialIndex);
	}
	if (m_fbxNoTangents) {
		CalculateTangents();
	}

	// Go through each node attached to this node and load it, then append their meshes to this mode's mesh list
	for (u32 i = 0; i < node->mNumChildren; ++i) {
		LoadNode(node->mChildren[i]);
	}
}

void AssimpLoader::LoadMesh(const aiMesh* aimesh, u32 meshIndex) {
	// only triangle mesh supported
	if (!(aimesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE)) {
		ENGINE_ERROR("FbxBuilder::LoadVertexData: only triangle meshes are supported");
		return;
	}

	const u32 numVertices = aimesh->mNumVertices;
	const u32 numFaces    = aimesh->mNumFaces;
	const u32 numIndices  = numFaces * 3;  // 3 indices per triangle a.k.a face

	size_t numVerticesBefore = vertices.size();
	size_t numIndicesBefore  = indices.size();

	// Resize vertex list to hold all vertices for mesh
	vertices.resize(numVerticesBefore + numVertices);
	indices.resize(numIndicesBefore + numIndices);

	Mesh& mesh       = meshes[meshIndex];
	mesh.firstVertex = static_cast<u32>(numVerticesBefore);
	mesh.firstIndex  = static_cast<u32>(numIndicesBefore);
	mesh.vertexCount = numVertices;
	mesh.indexCount  = numIndices;
	// mesh.instanceCount = m_InstanceCount;

	u32 vertexIndex = static_cast<u32>(numVerticesBefore);

	bool hasPositions = aimesh->HasPositions();
	bool hasNormals   = aimesh->HasNormals();
	bool hasTangents  = aimesh->HasTangentsAndBitangents();
	bool hasUVs       = aimesh->HasTextureCoords(0);
	bool hasColors    = aimesh->HasVertexColors(0);

	// Go through each vertex and copy it across to our vertices
	for (size_t i = 0; i < aimesh->mNumVertices; ++i) {
		Vertex& vertex = vertices[vertexIndex];

		// Set position
		if (hasPositions) {
			vertex.position = {aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z};
		}

		// Set normal
		if (hasNormals) {
			vertex.normal = {aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z};
		}

		// Set uv coords (if they exist)
		if (hasUVs) {
			vertex.uv = {aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y};
		} else {
			vertex.uv = {0.0f, 0.0f};
		}

		// Set tangent
		if (hasTangents) {
			vertex.tangent = {aimesh->mTangents[i].x, aimesh->mTangents[i].y, aimesh->mTangents[i].z};
		}

		// vertex colors
		{
			glm::vec4 vertexColor;
			u32 materialIndex = aimesh->mMaterialIndex;
			if (hasColors) {
				aiColor4D& colorFbx   = aimesh->mColors[0][i];
				glm::vec3 linearColor = glm::pow(glm::vec3(colorFbx.r, colorFbx.g, colorFbx.b), glm::vec3(2.2f));
				vertexColor           = glm::vec4(linearColor.r, linearColor.g, linearColor.b, colorFbx.a);
				vertex.color          = vertexColor * materials[materialIndex].pbrMaterial.diffuseColor;
			} else {
				vertex.color = materials[materialIndex].pbrMaterial.diffuseColor;
			}
		}

		++vertexIndex;
	}

	u32 index = static_cast<u32>(numIndicesBefore);
	// Iterate over indices through faces and copy across
	for (size_t i = 0; i < aimesh->mNumFaces; ++i) {
		// Get a face
		aiFace face = aimesh->mFaces[i];

		// Go thorugh face's indices and add to list
		for (size_t j = 0; j < face.mNumIndices; ++j) {
			indices[index + j] = face.mIndices[j];
		}
		index += 3;
	}

	ENGINE_INFO("Mesh loaded (Assimp): {0} vertices, {1} indices", numVertices, numIndices);

	// bone indices and bone weights
	{
		u32 numberOfBones = aimesh->mNumBones;
		std::vector<u32> numberOfBonesBoundtoVertex;
		numberOfBonesBoundtoVertex.resize(vertices.size(), 0);
		for (u32 boneIndex = 0; boneIndex < numberOfBones; ++boneIndex) {
			aiBone& bone        = *aimesh->mBones[boneIndex];
			u32 numberOfWeights = bone.mNumWeights;

			// loop over vertices that are bound to that bone
			for (u32 weightIndex = 0; weightIndex < numberOfWeights; ++weightIndex) {
				u32 vertexId = bone.mWeights[weightIndex].mVertexId;
				ENGINE_ASSERT(vertexId < vertices.size(), "memory violation");
				float weight = bone.mWeights[weightIndex].mWeight;
				switch (numberOfBonesBoundtoVertex[vertexId]) {
					case 0:
						vertices[vertexId].jointIds.x = boneIndex;
						vertices[vertexId].weights.x  = weight;
						break;
					case 1:
						vertices[vertexId].jointIds.y = boneIndex;
						vertices[vertexId].weights.y  = weight;
						break;
					case 2:
						vertices[vertexId].jointIds.z = boneIndex;
						vertices[vertexId].weights.z  = weight;
						break;
					case 3:
						vertices[vertexId].jointIds.w = boneIndex;
						vertices[vertexId].weights.w  = weight;
						break;
					default:
						break;
				}
				// track how many times this bone was hit
				// (up to four bones can be bound to a vertex)
				++numberOfBonesBoundtoVertex[vertexId];
			}
		}

		// normalize weights
		for (u32 vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex) {
			glm::vec4& boneWeights = vertices[vertexIndex].weights;
			float weightSum        = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
			if (weightSum > std::numeric_limits<float>::epsilon()) {
				vertices[vertexIndex].weights = glm::vec4(
					boneWeights.x / weightSum, boneWeights.y / weightSum, boneWeights.z / weightSum, boneWeights.w / weightSum
				);
			}
		}
	}
}

void AssimpLoader::AssignMaterial(Mesh& mesh, const int materialIndex) {
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

void AssimpLoader::CalculateTangents() {
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

void AssimpLoader::CalculateTangentsFromIndexBuffer(const std::vector<u32>& indices) {
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