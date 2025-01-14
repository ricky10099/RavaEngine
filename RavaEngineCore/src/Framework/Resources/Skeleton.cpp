#include "ravapch.h"

#include "Framework/Resources/Skeleton.h"

namespace Rava {
void Skeleton::Traverse() {
	ENGINE_TRACE("Skeleton: {0}", name);
	u32 indent = 0;
	std::string indentStr(indent, ' ');
	auto& joint = joints[0];  // root joint
	Traverse(joint, indent + 1);
}
void Skeleton::Traverse(Joint const& joint, u32 indent) {
	std::string indentStr(indent, ' ');
	size_t numberOfChildren = joint.children.size();

	ENGINE_TRACE(
		"{0}name: {1}, m_Parent: {2}, m_Children.size(): {3}", indentStr, joint.name, joint.parentJoint, numberOfChildren
	);

	for (size_t childIndex = 0; childIndex < numberOfChildren; ++childIndex) {
		int jointIndex = joint.children[childIndex];
		ENGINE_TRACE("{0}child {1}: index: {2}", indentStr, childIndex, jointIndex);
	}

	for (size_t childIndex = 0; childIndex < numberOfChildren; ++childIndex) {
		int jointIndex = joint.children[childIndex];
		Traverse(joints[jointIndex], indent + 1);
	}
}

void Skeleton::Update() {
	// update the final global transform of all joints
	int16_t numberOfJoints = static_cast<int16_t>(joints.size());

	if (!isAnimated)  // used for debugging to check if the model renders w/o deformation
	{
		for (int16_t jointIndex = 0; jointIndex < numberOfJoints; ++jointIndex) {
			skeletonUbo.jointsMatrices[jointIndex] = glm::mat4(1.0f);
		}
	} else {
		// STEP 1: apply animation results
		for (int16_t jointIndex = 0; jointIndex < numberOfJoints; ++jointIndex) {
			skeletonUbo.jointsMatrices[jointIndex] = joints[jointIndex].GetDeformedBindMatrix();
		}

		// STEP 2: recursively update final joint matrices
		UpdateJoint(ROOT_JOINT);

		// STEP 3: bring back into model space
		for (int16_t jointIndex = 0; jointIndex < numberOfJoints; ++jointIndex) {
			skeletonUbo.jointsMatrices[jointIndex] =
				skeletonUbo.jointsMatrices[jointIndex] * joints[jointIndex].inverseBindMatrix;
		}
	}
}

// Update the final joint matrices of all joints
// traverses entire skeleton from top (a.k.a root a.k.a hip bone)
// This way, it is guaranteed that the global parent transform is already updated
void Skeleton::UpdateJoint(int16_t jointIndex) {
	auto& currentJoint = joints[jointIndex];  // just a reference for easier code

	int16_t parentJoint = currentJoint.parentJoint;
	if (parentJoint != NO_PARENT) {
		skeletonUbo.jointsMatrices[jointIndex] = skeletonUbo.jointsMatrices[parentJoint] * skeletonUbo.jointsMatrices[jointIndex];
	}

	// update children
	size_t numberOfChildren = currentJoint.children.size();
	for (size_t childIndex = 0; childIndex < numberOfChildren; ++childIndex) {
		int childJoint = currentJoint.children[childIndex];
		UpdateJoint(childJoint);
	}
}
}  // namespace Rava