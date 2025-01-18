#pragma once

#include "Framework/Resources/MeshModel.h"

namespace Vulkan {
class Buffer;
}

namespace Rava {
class Texture;
class Material;
class Animations;
struct Skeleton;
class AssimpLoader {
   public:
	std::vector<u32> indices{};
	std::vector<Vertex> vertices{};
	std::vector<Mesh> meshes{};
	std::vector<Material> materials{};

   public:
	AssimpLoader() = delete;
	AssimpLoader(const std::string& filePath);
	~AssimpLoader() = default;

	bool LoadModel(const u32 instanceCount = 1);

   private:
	std::string m_filePath;
	std::string m_path;

	u32 m_loadingFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
					   | aiProcess_LimitBoneWeights | aiProcess_CalcTangentSpace;
	aiScene* m_modelScene = nullptr;
	std::unordered_map<std::string, u32> m_materialNameToIndex;

	u32 m_instanceCount = 1;
	u32 m_instanceIndex = 0;

	bool m_fbxNoTangents;

   private:
	void LoadMaterials();
	void LoadProperties(const aiMaterial* fbxMaterial, Material::PBRMaterial& pbrMaterial);
	void LoadMap(const aiMaterial* fbxMaterial, aiTextureType textureType, int materialIndex);
	// void LoadMaterial(const ufbx_material* fbxMaterial, ufbx_material_pbr_map materialProperty, int materialIndex);
	std::shared_ptr<Texture> LoadTexture(const std::string& filepath, bool useSRGB);
	void LoadNode(aiNode* node);
	void LoadMesh(const aiMesh* fbxMesh, const u32 meshIndex);
	void AssignMaterial(Mesh& mesh, const int materialIndex);

	void CalculateTangentsFromIndexBuffer(const std::vector<u32>& indices);
	void CalculateTangents();

   public:
	Shared<Skeleton> skeleton;
	Shared<Vulkan::Buffer> skeletonUbo;
	// Unique<Animations> animations;

	//  public:
	// bool LoadAnimations();
	// bool AddAnimation();

   private:
	//void LoadSkeletons();
	// void LoadAnimationClips();
	// void AddAnimationClip();
};
}  // namespace Rava