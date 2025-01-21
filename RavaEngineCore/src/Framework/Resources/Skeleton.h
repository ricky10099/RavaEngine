#pragma once

namespace Rava {
static constexpr int NO_PARENT  = -1;
static constexpr int ROOT_JOINT = 0;

struct SkeletonUbo {
	std::vector<glm::mat4> jointsMatrices;
};

struct Joint {
	std::string_view name;
	glm::mat4 inverseBindMatrix;

	// deformed / animated
	// to be applied to the node matrix a.k.a bind matrix in the world coordinate system,
	// controlled by an animation or a single pose (they come out of gltf animation samplers)
	glm::vec3 deformedNodeTranslation{0.0f};                             // T
	glm::quat deformedNodeRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // R
	glm::vec3 deformedNodeScale{1.0f};                                   // S

	glm::mat4 GetDeformedBindMatrix() const {
		// apply scale, rotation, and translation IN THAT ORDER (read from right to the left)
		// to the original undefomed bind matrix
		// dynamically called once per frame
		return glm::scale(glm::mat4(1.0f), deformedNodeScale) *           // S
			   glm::mat4(deformedNodeRotation) *                          // R
			   glm::translate(glm::mat4(1.0f), deformedNodeTranslation);  // T
	}

	// parents and children for the tree hierachy
	int parentJoint;
	std::vector<int> children;
};

struct Bone {
	std::string name;
	i32 parentIndex;
	std::vector<Bone> children;
	glm::mat4 offsetMatrix;
	glm::mat4 localTransform{1.0f};
	glm::mat4 globalTransform{1.0f};
};

struct Skeleton {
	void Update();

	bool isAnimated = true;
	std::string_view name;
	//std::vector<Joint> joints;
	std::vector<Bone> bones;
	std::map<std::string, i32> boneMap;
	std::map<int, int> globalNodeToJointIndex;
	SkeletonUbo skeletonUbo;
};
}  // namespace Rava