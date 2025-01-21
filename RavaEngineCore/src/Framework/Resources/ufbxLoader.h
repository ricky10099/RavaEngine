#pragma once

#include <ufbx/ufbx.h>

#include "Framework/Resources/MeshModel.h"
#include "Framework/Resources/Animations.h"

namespace Vulkan {
class Buffer;
}

namespace Rava {
class Texture;
class Material;
struct Skeleton;
class ufbxLoader {
   public:
	std::vector<u32> indices{};
	std::vector<Vertex> vertices{};
	std::vector<Mesh> meshes{};
	//std::vector<Node> nodes{};
	std::vector<Material> materials{};
	std::map<std::string, i32> nodeMap;
	std::map<std::string, u32> boneMap;

   public:
	ufbxLoader() = delete;
	ufbxLoader(const std::string& filePath);
	~ufbxLoader() = default;

	bool LoadModel(const u32 instanceCount = 1);

   private:
	std::string m_filePath;
	std::string m_path;
	ufbx_scene* m_modelScene = nullptr;
	std::unordered_map<std::string, u32> m_materialNameToIndex;

	u32 m_instanceCount = 1;
	u32 m_instanceIndex = 0;

	bool m_fbxNoTangents = false;

   private:
	void LoadMaterials();
	void LoadMaterial(const ufbx_material* fbxMaterial, ufbx_material_pbr_map materialProperty, int materialIndex);
	std::shared_ptr<Texture> LoadTexture(ufbx_material_map const& materialMap, bool useSRGB);
	void LoadNode(const ufbx_node* fbxNode);
	void LoadMesh(const ufbx_node* fbxNode, const u32 meshIndex);
	void AssignMaterial(Mesh& submesh, int const materialIndex);

	void CalculateTangentsFromIndexBuffer(const std::vector<u32>& indices);
	void CalculateTangents();

	glm::mat4 ufbxToglm(const ufbx_matrix& ufbxMat);
	glm::vec3 ufbxToglm(const ufbx_vec3& ufbxVec3);
	glm::quat ufbxToglm(const ufbx_quat& ufbxQuat);

   public:
	Shared<Skeleton> skeleton;
	Shared<Vulkan::Buffer> skeletonUbo;
	Unique<Animations> animations;

   public:
	bool LoadAnimations();
	bool AddAnimation();

   private:
	void LoadSkeletons();
	void LoadAnimationClips();
	// void AddAnimationClip();
};
}  // namespace Rava