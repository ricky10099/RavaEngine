#include "ravapch.h"

#include "Framework/Resources/AssimpLoader.h"

namespace Rava {
void AssimpLoader::LoadSkeleton(aiNode* aiNode, aiMatrix4x4 parentTransform) {
	aiMatrix4x4 transform = parentTransform * aiNode->mTransformation;

	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		model->meshes.push_back(ModelInfo::Mesh());
		processMesh(model, mesh, scene, transform);
	}

	// bones - relies on verticies
	for (u32 boneIndex = 0; boneIndex < aimesh->mNumBones; ++boneIndex) {
		auto aibone = aimesh->mBones[i];
		unsigned int boneID;
		std::string boneName = aibone->mName.C_Str();
		if (model->boneMap.find(boneName) == model->boneMap.end()) {
			model->bones.push_back(aiToGLM(aibone->mOffsetMatrix) * mesh->bindTransform);
			boneID                   = (unsigned int)(model->bones.size() - 1);
			model->boneMap[boneName] = boneID;
		} else {
			boneID = model->boneMap[boneName];
		}

		for (unsigned int weightI = 0; weightI < aibone->mNumWeights; weightI++) {
			auto vertexWeight = aibone->mWeights[weightI];
			mesh->verticies[vertexWeight.mVertexId].BoneIDs.push_back(boneID == -1 ? 0 : boneID);
			mesh->verticies[vertexWeight.mVertexId].BoneWeights.push_back(vertexWeight.mWeight);
		}
	}
}
}  // namespace Rava