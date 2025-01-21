#include "ravapch.h"

#include "Framework/Resources/Skeleton.h"

namespace Rava {
void Skeleton::Update() {
	for (size_t boneIndex = 0; boneIndex < bones.size(); boneIndex++) {
		Bone& bone = bones[boneIndex];

		// ufbx stores nodes in order where parent nodes always precede child nodes so we can
		// evaluate the transform hierarchy with a flat loop.
		if (bone.parentIndex >= 0) {
			bone.globalTransform = bones[bone.parentIndex].globalTransform * bone.localTransform;
		} else {
			bone.globalTransform = bone.localTransform;
		}
		skeletonUbo.jointsMatrices[boneIndex] = bone.globalTransform * bone.offsetMatrix;
	}
}
}  // namespace Rava