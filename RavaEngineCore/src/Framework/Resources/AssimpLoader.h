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
	std::vector<Node> nodes{};
	std::vector<Material> materials{};
	std::map<std::string, i32> nodeMap;

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
	void MarkNode(const aiNode* fbxNode, aiMatrix4x4 parentTransform, i32 parentNode);
	void LoadNode(aiNode* node);
	void LoadMesh(const aiMesh* fbxMesh, const u32 meshIndex);
	void AssignMaterial(Mesh& mesh, const int materialIndex);

	void CalculateTangentsFromIndexBuffer(const std::vector<u32>& indices);
	void CalculateTangents();

	glm::mat4 aiToglm(aiMatrix4x4 aiMat) {
		glm::mat4 glmMat{};
		glmMat[0][0] = aiMat.a1;
		glmMat[0][1] = aiMat.b1;
		glmMat[0][2] = aiMat.c1;
		glmMat[0][3] = aiMat.d1;

		glmMat[1][0] = aiMat.a2;
		glmMat[1][1] = aiMat.b2;
		glmMat[1][2] = aiMat.c2;
		glmMat[1][3] = aiMat.d2;

		glmMat[2][0] = aiMat.a3;
		glmMat[2][1] = aiMat.b3;
		glmMat[2][2] = aiMat.c3;
		glmMat[2][3] = aiMat.d3;

		glmMat[3][0] = aiMat.a4;
		glmMat[3][1] = aiMat.b4;
		glmMat[3][2] = aiMat.c4;
		glmMat[3][3] = aiMat.d4;

		return glmMat;
	}

   public:
	Shared<Skeleton> skeleton;
	Shared<Vulkan::Buffer> skeletonUbo;
	// Unique<Animations> animations;

	//  public:
	// bool LoadAnimations();
	// bool AddAnimation();

   private:
	void LoadSkeleton(aiMatrix4x4 transform);
	// void LoadAnimationClips();
	// void AddAnimationClip();
};
}  // namespace Rava