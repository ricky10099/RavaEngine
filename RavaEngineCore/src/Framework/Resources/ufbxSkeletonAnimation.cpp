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
	//{
	//					  .right = UFBX_COORDINATE_AXIS_POSITIVE_X,
	//					  .up    = UFBX_COORDINATE_AXIS_POSITIVE_Y,
	//					  .front = UFBX_COORDINATE_AXIS_POSITIVE_Z,
	//   };
	loadOptions.target_unit_meters = 1.0f;

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

	LoadAnimationClips();

	ufbx_free_scene(m_modelScene);
	return true;
}

bool ufbxLoader::AddAnimation() {
	ufbx_load_opts loadOptions{.ignore_geometry = true};
	loadOptions.load_external_files           = true;
	loadOptions.ignore_missing_external_files = true;
	loadOptions.generate_missing_normals      = true;
	loadOptions.target_axes                   = {
						  .right = UFBX_COORDINATE_AXIS_POSITIVE_X,
						  .up    = UFBX_COORDINATE_AXIS_POSITIVE_Y,
						  .front = UFBX_COORDINATE_AXIS_POSITIVE_Z,
    };
	loadOptions.target_unit_meters = 1.0f;

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

	AddAnimationClip();

	ufbx_free_scene(m_modelScene);
	return true;
}

void ufbxLoader::LoadSkeletons() {
	u32 numberOfSkeletons = 0;
	u32 meshIndex         = 0;
	size_t numberOfBones  = 0;
	skeleton              = std::make_shared<Skeleton>();

	auto ufbxMatToGlm = [](ufbx_matrix const& ufbxMat) {
		glm::mat4 glmMat4{};
		for (u32 column = 0; column < 4; ++column) {
			glmMat4[column].x = ufbxMat.cols[column].x;
			glmMat4[column].y = ufbxMat.cols[column].y;
			glmMat4[column].z = ufbxMat.cols[column].z;
			glmMat4[column].w = column < 3 ? 0.0f : 1.0f;
		}
		return glmMat4;
	};

	for (size_t i = 0; i < m_modelScene->nodes.count; ++i) {
		if (m_modelScene->nodes[i]->bone) {
			Bone bone{};
			i32 parent      = m_modelScene->nodes[i]->parent ? m_modelScene->nodes[i]->parent->typed_id : -1;
			bone.parent     = parent;
			bone.transform  = ufbxMatToGlm(m_modelScene->nodes[i]->node_to_world);
			bone.boneOffset = ufbxMatToGlm(m_modelScene->nodes[i]->node_to_parent);
			bone.boneID     = m_modelScene->nodes[i]->typed_id;
			if (parent >= 0) {
				skeleton->bones[parent].children.push_back(bone.boneID);
			}
			skeleton->bones.push_back(bone);
			++numberOfBones;
		}
	}

	size_t bufferSize = numberOfBones * sizeof(glm::mat4);  // in bytes
	skeletonUbo       = std::make_shared<Vulkan::Buffer>(bufferSize);
	skeletonUbo->Map();
}

void ufbxLoader::LoadAnimationClips() {
	u32 numberOfSkeletons = 0;
	u32 meshIndex         = 0;
	// iterate over all meshes and check if they have a skeleton
	for (u32 index = 0; index < m_modelScene->meshes.count; ++index) {
		ufbx_mesh& mesh = *m_modelScene->meshes.data[index];
		if (mesh.skin_deformers.count) {
			++numberOfSkeletons;
			meshIndex = index;
		}
	}

	if (!numberOfSkeletons) {
		return;
	}

	if (numberOfSkeletons > 1) {
		ENGINE_WARN("A model should only have a single skin/armature/skeleton. Using mesh {0}.", numberOfSkeletons - 1);
	}

	std::unordered_map<std::string, int> nameToBoneIndex;

	// load skeleton
	{
		ufbx_mesh& mesh             = *m_modelScene->meshes.data[meshIndex];
		ufbx_skin_deformer& fbxSkin = *mesh.skin_deformers.data[0];
		size_t numberOfBones        = fbxSkin.clusters.count;

		for (u32 boneIndex = 0; boneIndex < numberOfBones; ++boneIndex) {
			ufbx_skin_cluster& bone   = *fbxSkin.clusters.data[boneIndex];
			std::string boneName      = bone.name.data;
			nameToBoneIndex[boneName] = boneIndex;
		}
	}

	auto ufbxVec3ToGlm = [](ufbx_vec3 const& ufbxVec3) { return glm::vec3(ufbxVec3.x, ufbxVec3.y, ufbxVec3.z); };
	auto ufbxQuatToGlm = [](ufbx_quat const& ufbxQuat) {
		glm::quat glmQuat{};
		glmQuat.x = ufbxQuat.x;
		glmQuat.y = ufbxQuat.y;
		glmQuat.z = ufbxQuat.z;
		glmQuat.w = ufbxQuat.w;
		return glmQuat;
	};

	animations = std::make_unique<Animations>();

	size_t numberOfAnimations = m_modelScene->anim_stacks.count;
	for (size_t animationIndex = 0; animationIndex < numberOfAnimations; ++animationIndex) {
		ufbx_anim_stack& fbxAnimation = *m_modelScene->anim_stacks.data[animationIndex];
		std::string_view animationName(fbxAnimation.name.data);

		float duration   = (float)fbxAnimation.time_end - (float)fbxAnimation.time_begin;
		size_t numFrames = glm::clamp((duration * 30), 2.0f, 4096.0f);
		float framerate  = (float)(numFrames - 1) / duration;

		if (animationName.find("|") != std::string::npos) {
			continue;
		}
		ENGINE_INFO("name of animation: {0}", animationName);

		Shared<AnimationClip> animation = std::make_shared<AnimationClip>(animationName);
		animation->SetFirstKeyFrameTime(fbxAnimation.time_begin);
		animation->SetLastKeyFrameTime(fbxAnimation.time_end);
		animation->animNodes.resize(skeleton->bones.size());
		for (size_t i = 0; i < skeleton->bones.size(); ++i) {
			ufbx_node* node = m_modelScene->nodes.data[i];
			animation->animNodes[i].positions.resize(numFrames);
			animation->animNodes[i].rotations.resize(numFrames);
			animation->animNodes[i].scales.resize(numFrames);
			bool const_rot = true, const_pos = true, const_scale = true;

			for (size_t j = 0; j < numFrames; ++j) {
				double time = fbxAnimation.time_begin + (double)j / framerate;

				ufbx_transform transform = ufbx_evaluate_transform(fbxAnimation.anim, m_modelScene->nodes.data[j], time);
				animation->animNodes[i].positions[j].xyz        = ufbxVec3ToGlm(transform.translation);
				animation->animNodes[i].rotations[j].quaternion = ufbxQuatToGlm(transform.rotation);
				animation->animNodes[i].scales[j].xyz           = ufbxVec3ToGlm(transform.scale);

				if (j > 0) {
					if (glm::dot(
							animation->animNodes[i].rotations[j].quaternion, animation->animNodes[i].rotations[j - 1].quaternion
						)
						< 0.0f) {
						animation->animNodes[i].rotations[j].quaternion *= -1;
					}

					if (!(animation->animNodes[i].rotations[j].quaternion == animation->animNodes[i].rotations[j - 1].quaternion
						)) {
						const_rot = false;
					}

					if (!(animation->animNodes[i].positions[j].xyz == animation->animNodes[i].positions[j - 1].xyz)) {
						const_pos = false;
					}

					if (!(animation->animNodes[i].scales[j].xyz == animation->animNodes[i].scales[j - 1].xyz)) {
						const_scale = false;
					}
				}
			}
		}

		animations->Push(animation);
	}
	// m_SkeletalAnimation = (m_Animations->Size()) ? true : false;
}

void ufbxLoader::AddAnimationClip() {
	u32 numberOfSkeletons = 0;
	u32 meshIndex         = 0;
	// iterate over all meshes and check if they have a skeleton
	for (u32 index = 0; index < m_modelScene->meshes.count; ++index) {
		ufbx_mesh& mesh = *m_modelScene->meshes.data[index];
		if (mesh.skin_deformers.count) {
			++numberOfSkeletons;
			meshIndex = index;
		}
	}

	if (!numberOfSkeletons) {
		return;
	}

	if (numberOfSkeletons > 1) {
		ENGINE_WARN("A model should only have a single skin/armature/skeleton. Using mesh {0}.", numberOfSkeletons - 1);
	}

	std::unordered_map<std::string, int> nameToBoneIndex;

	// load skeleton
	{
		ufbx_mesh& mesh             = *m_modelScene->meshes.data[meshIndex];
		ufbx_skin_deformer& fbxSkin = *mesh.skin_deformers.data[0];
		size_t numberOfBones        = fbxSkin.clusters.count;

		for (u32 boneIndex = 0; boneIndex < numberOfBones; ++boneIndex) {
			ufbx_skin_cluster& bone   = *fbxSkin.clusters.data[boneIndex];
			std::string boneName      = bone.name.data;
			nameToBoneIndex[boneName] = boneIndex;
		}
	}

	animations = std::make_unique<Animations>();

	size_t numberOfAnimations = m_modelScene->anim_stacks.count;
	for (size_t animationIndex = 0; animationIndex < numberOfAnimations; ++animationIndex) {
		ufbx_anim_stack& fbxAnimation = *m_modelScene->anim_stacks.data[animationIndex];

		std::string animationName(fbxAnimation.name.data);
		// the fbx includes animations twice,
		// as "armature|name" and "name"
		if (animationName.find("|") != std::string::npos) {
			continue;
		}

		ENGINE_INFO("name of animation: {0}", animationName);

		Shared<AnimationClip> animation = std::make_shared<AnimationClip>(animationName);

		animation->SetFirstKeyFrameTime(fbxAnimation.time_begin);
		animation->SetLastKeyFrameTime(fbxAnimation.time_end);

		ufbx_bake_opts bakeOptions = {};
		ufbx_error ufbxError{};
		ufbx_unique_ptr<ufbx_baked_anim> fbxBakedAnim{ufbx_bake_anim(m_modelScene, fbxAnimation.anim, &bakeOptions, &ufbxError)};

		if (!fbxBakedAnim) {
			char errorBuffer[512];
			ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
			ENGINE_ASSERT(false, "failed to bake animation, " + std::string(errorBuffer));
			return;
		}

		// helper lambdas to convert asset importer formats to glm
		auto vec3UfbxToGlm           = [](ufbx_vec3 const& vec3Ufbx) { return glm::vec3(vec3Ufbx.x, vec3Ufbx.y, vec3Ufbx.z); };
		auto quaternionUfbxToGlmVec4 = [](ufbx_quat const& quaternionUfbx) {
			glm::vec4 vec4GLM{};
			vec4GLM.x = (float)quaternionUfbx.x;
			vec4GLM.y = (float)quaternionUfbx.y;
			vec4GLM.z = (float)quaternionUfbx.z;
			vec4GLM.w = (float)quaternionUfbx.w;
			return vec4GLM;
		};

		u32 channelAndSamplerIndex = 0;
		for (const ufbx_baked_node& fbxChannel : fbxBakedAnim->nodes) {
			const u32 nodeIndex = fbxChannel.typed_id;
			std::string fbxChannelName(m_modelScene->nodes[nodeIndex]->name.data);
			// use fbx channels that actually belong to bones
			bool isBone = nameToBoneIndex.contains(fbxChannelName);
			if (isBone) {
				// Each node of the skeleton has channels that point to samplers
				{  // set up channels
					{
						AnimationClip::Channel channel{};
						channel.path         = AnimationClip::Path::TRANSLATION;
						channel.samplerIndex = channelAndSamplerIndex + 0;
						channel.node         = nameToBoneIndex[fbxChannelName];

						animation->channels.push_back(channel);
					}
					{
						AnimationClip::Channel channel{};
						channel.path         = AnimationClip::Path::ROTATION;
						channel.samplerIndex = channelAndSamplerIndex + 1;
						channel.node         = nameToBoneIndex[fbxChannelName];

						animation->channels.push_back(channel);
					}
					{
						AnimationClip::Channel channel{};
						channel.path         = AnimationClip::Path::SCALE;
						channel.samplerIndex = channelAndSamplerIndex + 2;
						channel.node         = nameToBoneIndex[fbxChannelName];

						animation->channels.push_back(channel);
					}
				}

				{  // set up samplers
					{
						u32 numberOfKeys = fbxChannel.translation_keys.count;

						AnimationClip::Sampler sampler;
						sampler.timestamps.resize(numberOfKeys);
						sampler.valuesToInterpolate.resize(numberOfKeys);
						sampler.interpolation = AnimationClip::InterpolationMethod::LINEAR;
						for (u32 key = 0; key < numberOfKeys; ++key) {
							ufbx_vec3& value                 = fbxChannel.translation_keys.data[key].value;
							sampler.valuesToInterpolate[key] = glm::vec4(vec3UfbxToGlm(value), 0.0f);
							sampler.timestamps[key]          = fbxChannel.translation_keys.data[key].time;
						}

						animation->samplers.push_back(sampler);
					}
					{
						u32 numberOfKeys = fbxChannel.rotation_keys.count;

						AnimationClip::Sampler sampler;
						sampler.timestamps.resize(numberOfKeys);
						sampler.valuesToInterpolate.resize(numberOfKeys);
						sampler.interpolation = AnimationClip::InterpolationMethod::LINEAR;
						for (u32 key = 0; key < numberOfKeys; ++key) {
							ufbx_quat& value                 = fbxChannel.rotation_keys.data[key].value;
							sampler.valuesToInterpolate[key] = quaternionUfbxToGlmVec4(value);
							sampler.timestamps[key]          = fbxChannel.rotation_keys.data[key].time;
						}

						animation->samplers.push_back(sampler);
					}
					{
						u32 numberOfKeys = fbxChannel.scale_keys.count;

						AnimationClip::Sampler sampler;
						sampler.timestamps.resize(numberOfKeys);
						sampler.valuesToInterpolate.resize(numberOfKeys);
						sampler.interpolation = AnimationClip::InterpolationMethod::LINEAR;
						for (u32 key = 0; key < numberOfKeys; ++key) {
							ufbx_vec3& value                 = fbxChannel.scale_keys.data[key].value;
							sampler.valuesToInterpolate[key] = glm::vec4(vec3UfbxToGlm(value), 0.0f);
							sampler.timestamps[key]          = fbxChannel.scale_keys.data[key].time;
						}

						animation->samplers.push_back(sampler);
					}
				}
				channelAndSamplerIndex += 3;
			}
		}

		animations->Push(animation);
	}

	// m_SkeletalAnimation = (m_Animations->Size()) ? true : false;
}
}  // namespace Rava