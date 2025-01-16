#include "ravapch.h"

#include "Framework/RavaUtils.h"
#include "Framework/Resources/AssimpLoader.h"

namespace Rava {
AssimpLoader::AssimpLoader(const std::string& filePath)
	: m_filePath(filePath) {
	m_path = GetPathWithoutFileName(filePath);
}

bool AssimpLoader::LoadModel(const u32 instanceCount) {
	Assimp::Importer importer;

	const aiScene* m_modelScene = importer.ReadFile(m_filePath.c_str(), m_loadingFlags);

	if (!m_modelScene) {
		ENGINE_ERROR("Failed to load model! (" + m_filePath + ")");
	}


	LoadNode(m_modelScene->mRootNode, aiMatrix4x4(), -1);
}

void AssimpLoader::LoadNode(aiNode* node, aiMatrix4x4 parentTransform, int parentNode) {
	aiMatrix4x4 transform = parentTransform * node->mTransformation;

	for (u32 i = 0; i < node->mNumMeshes; ++i) {
		aiMesh* fbxMesh = m_modelScene->mMeshes[node->mMeshes[i]];
		meshes.push_back(Mesh());
		LoadMesh(fbxMesh, i);
	}
}

void AssimpLoader::LoadMesh(const aiMesh* fbxMesh, u32 meshIndex) {
	Mesh& mesh = meshes[meshes.size() - 1];

}

}  // namespace Rava