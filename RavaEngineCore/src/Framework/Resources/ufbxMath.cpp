#include "ravapch.h"

#include "Framework/Resources/ufbxLoader.h"

namespace Rava {
glm::mat4 ufbxLoader::ufbxToglm(const ufbx_matrix& ufbxMat) {
	glm::mat4 glmMat4{};
	for (u32 column = 0; column < 4; ++column) {
		glmMat4[column].x = ufbxMat.cols[column].x;
		glmMat4[column].y = ufbxMat.cols[column].y;
		glmMat4[column].z = ufbxMat.cols[column].z;
		glmMat4[column].w = column < 3 ? 0.0f : 1.0f;
	}
	return glmMat4;
}

glm::vec3 ufbxLoader::ufbxToglm(const ufbx_vec3& ufbxVec3) {
	return glm::vec3(ufbxVec3.x, ufbxVec3.y, ufbxVec3.z);
}

glm::quat ufbxLoader::ufbxToglm(const ufbx_quat& ufbxQuat) {
	glm::quat quatGLM{};
	quatGLM.x = ufbxQuat.x;
	quatGLM.y = ufbxQuat.y;
	quatGLM.z = ufbxQuat.z;
	quatGLM.w = ufbxQuat.w;
	return quatGLM;
}
}  // namespace Rava