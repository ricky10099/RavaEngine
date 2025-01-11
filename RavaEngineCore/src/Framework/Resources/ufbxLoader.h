#pragma once

#include <ufbx/ufbx.h>

#include "Framework/Resources/MeshModel.h"

namespace Rava {
class ufbxLoader {
   public:
	std::vector<u32> indices{};
	std::vector<Vertex> vertices{};
	std::vector<Mesh> meshes{};

   public:
	ufbxLoader() = delete;
	ufbxLoader(const std::string& filePath);
	~ufbxLoader() = default;

	bool Load(const u32 instanceCount = 1);

   private:
	std::string m_filePath;
	std::string m_path;
	ufbx_scene* m_modelScene = nullptr;

	u32 m_instanceCount = 1;
	u32 m_instanceIndex = 0;

	bool m_fbxNoTangents;

   private:
	void LoadNode(const ufbx_node* fbxNode);
	void LoadMesh(const ufbx_node* fbxNode, const u32 meshIndex);

	void CalculateTangentsFromIndexBuffer(const std::vector<u32>& indices);
	void CalculateTangents();
};
}  // namespace Rava