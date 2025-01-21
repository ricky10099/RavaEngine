#include "ravapch.h"

#include "Framework/RavaUtils.h"
#include "Framework/Resources/ufbxLoader.h"
#include "Framework/Resources/Skeleton.h"
#include "Framework/Resources/Animations.h"
#include "Framework/Resources/AnimationClip.h"
#include "Framework/Vulkan/Buffer.h"

namespace Rava {
bool ufbxLoader::LoadAnimations() {
	ufbx_load_opts loadOptions{};
	loadOptions.ignore_geometry               = true;
	loadOptions.load_external_files           = true;
	loadOptions.ignore_missing_external_files = true;
	loadOptions.generate_missing_normals      = true;
	loadOptions.target_axes                   = ufbx_axes_left_handed_y_up;
	loadOptions.target_unit_meters            = 1.0f;

	// load raw data of the file (can be fbx or obj)
	ufbx_error ufbxError;

	m_modelScene = ufbx_load_file(m_filePath.data(), &loadOptions, &ufbxError);

	if (m_modelScene == nullptr) {
		char errorBuffer[512];
		ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
		ENGINE_ERROR("ufbxLoader::Load error: file: {0}, error: {1}", m_filePath, errorBuffer);
		return false;
	}

	if (!m_modelScene->meshes.count) {
		ENGINE_ERROR("ufbxBuilder::Load: no meshes found in {0}", m_filePath);
		return false;
	}

	LoadSkeletons();
	LoadAnimationClips();

	ufbx_free_scene(m_modelScene);
	return true;
}

bool ufbxLoader::AddAnimation() {
	return false;
}

void ufbxLoader::LoadSkeletons() {
	u32 boneCount = 0;

	if (m_modelScene->bones.count == 0) {
		return;
	}

	skeleton = std::make_shared<Skeleton>();

	for (size_t nodeIndex = 0; nodeIndex < m_modelScene->nodes.count; ++nodeIndex) {
		ufbx_node* node = m_modelScene->nodes.data[nodeIndex];
		Bone bone          = {};
		bone.name          = node->name.data;
		bone.parentIndex             = node->parent ? skeleton->boneMap.at(node->parent->name.data) : -1;
		bone.localTransform          = ufbxToglm(node->node_to_parent);
		bone.globalTransform         = ufbxToglm(node->node_to_world);
		bone.offsetMatrix            = ufbxToglm(node->geometry_to_node);
		i32 boneIndex                = static_cast<int>(skeleton->bones.size());
		skeleton->boneMap[bone.name] = boneIndex;
		skeleton->bones.push_back(bone);
		if (bone.parentIndex != -1) {
			skeleton->bones[bone.parentIndex].children.push_back(bone);
		}
		++boneCount;
	}

	skeleton->skeletonUbo.jointsMatrices.resize(boneCount);

	if (boneCount != 0) {
		size_t bufferSize = boneCount * sizeof(glm::mat4);  // in bytes
		skeletonUbo       = std::make_shared<Vulkan::Buffer>(bufferSize);
		skeletonUbo->Map();
	}
}

void ufbxLoader::LoadAnimationClips() {
	animations                = std::make_unique<Animations>();
	size_t numberOfAnimations = m_modelScene->anim_stacks.count;
	for (size_t animationIndex = 0; animationIndex < numberOfAnimations; ++animationIndex) {
		ufbx_anim_stack& fbxAnimation = *m_modelScene->anim_stacks.data[animationIndex];

		const float target_framerate = 30.0f;
		const int max_frames         = 4096;

		// Sample the animation evenly at `target_framerate` if possible while limiting the maximum
		// number of frames to `max_frames` by potentially dropping FPS.
		float duration    = (float)fbxAnimation.time_end - (float)fbxAnimation.time_begin;
		int num_frames = glm::clamp((int)(duration * target_framerate), 2, max_frames);
		float framerate   = (float)(num_frames - 1) / duration;

		std::string_view animationName(fbxAnimation.name.data);

		Shared<AnimationClip> animation = std::make_shared<AnimationClip>(animationName);
		animation->SetFirstKeyFrameTime((float)fbxAnimation.time_begin);
		animation->SetLastKeyFrameTime((float)fbxAnimation.time_end);
		animation->SetFramerate(framerate);
		animation->SetTotalFrameCount(num_frames);

		animation->animNodesList.resize(skeleton->bones.size());
		for (size_t boneIndex = 0; boneIndex < m_modelScene->nodes.count; ++boneIndex) {
			ufbx_node* node                   = m_modelScene->nodes.data[boneIndex];
			AnimationClip::AnimNode& animNode = animation->animNodesList[boneIndex];

			animNode.rot.resize(num_frames);
			animNode.pos.resize(num_frames);
			animNode.scale.resize(num_frames);

			for (size_t frameIndex = 0; frameIndex < num_frames; frameIndex++) {
				double time = fbxAnimation.time_begin + (double)frameIndex / framerate;

				ufbx_transform transform   = ufbx_evaluate_transform(fbxAnimation.anim, node, time);
				animNode.rot[frameIndex]   = ufbxToglm(transform.rotation);
				animNode.pos[frameIndex]   = ufbxToglm(transform.translation);
				animNode.scale[frameIndex] = ufbxToglm(transform.scale);

				if (frameIndex > 0) {
					// Negated quaternions are equivalent, but interpolating between ones of different
					// polarity takes a the longer path, so flip the quaternion if necessary.
					if (glm::dot(animNode.rot[frameIndex], animNode.rot[frameIndex - 1]) < 0.0f) {
						animNode.rot[frameIndex] *= -1;
					}
				}
			}
		}

		animations->Push(animation);
	}
}
}  // namespace Rava