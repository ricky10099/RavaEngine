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

	LoadAnimationClips();

	ufbx_free_scene(m_modelScene);
	return true;
}

bool ufbxLoader::AddAnimation() {
	ufbx_load_opts loadOptions{ .ignore_geometry = true};
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

	// m_Animations = std::make_shared<SkeletalAnimations>();
	skeleton = std::make_shared<Skeleton>();
	std::unordered_map<std::string, int> nameToBoneIndex;

	// load skeleton
	{
		ufbx_mesh& mesh             = *m_modelScene->meshes.data[meshIndex];
		ufbx_skin_deformer& fbxSkin = *mesh.skin_deformers.data[0];
		size_t numberOfBones        = fbxSkin.clusters.count;
		auto& bones = skeleton->joints;  // just a reference to the bones std::vector of that skeleton (to make code easier)

		bones.resize(numberOfBones);
		skeleton->skeletonUbo.jointsMatrices.resize(numberOfBones);

		// set up map to find the names of Bones when traversing the node hierarchy
		// by iterating the clsuetrs array of the mesh
		for (u32 boneIndex = 0; boneIndex < numberOfBones; ++boneIndex) {
			ufbx_skin_cluster& bone   = *fbxSkin.clusters.data[boneIndex];
			std::string boneName      = bone.name.data;
			nameToBoneIndex[boneName] = boneIndex;

			// compatibility code with glTF loader; needed in skeletalAnimation.cpp
			// m_Channels.m_Node must be set up accordingly
			skeleton->globalNodeToJointIndex[boneIndex] = boneIndex;
		}

		// lambda to convert ufbx_matrix to glm::mat4
		auto mat4UfbxToGlm = [](ufbx_matrix const& mat4Ufbx) {
			glm::mat4 mat4Glm;
			for (u32 column = 0; column < 4; ++column) {
				mat4Glm[column].x = (float)mat4Ufbx.cols[column].x;
				mat4Glm[column].y = (float)mat4Ufbx.cols[column].y;
				mat4Glm[column].z = (float)mat4Ufbx.cols[column].z;
				mat4Glm[column].w = column < 3 ? 0.0f : 1.0f;
				
			}
			return mat4Glm;
		};

		// recursive lambda to traverse fbx node hierarchy
		std::function<void(ufbx_node*, u32)> traverseNodeHierarchy = [&](ufbx_node* node, u32 parent) {
			size_t numberOfChildren = node->children.count;
			u32 boneIndex           = parent;
			// does the node name correspond to a bone name?
			std::string nodeName = node->name.data;
			bool isBone          = nameToBoneIndex.contains(nodeName);

			if (isBone) {
				boneIndex                          = nameToBoneIndex[nodeName];
				bones[boneIndex].name              = nodeName;
				ufbx_skin_cluster& bone            = *fbxSkin.clusters.data[boneIndex];
				bones[boneIndex].inverseBindMatrix = mat4UfbxToGlm(bone.geometry_to_bone);
				ENGINE_TRACE(
					"\nBone: {} Inverse Bind Matrix: {}", bones[boneIndex].name, glm::to_string(bones[boneIndex].inverseBindMatrix)
				);
				bones[boneIndex].parentJoint       = parent;
			}
			for (u32 childIndex = 0; childIndex < numberOfChildren; ++childIndex) {
				if (isBone) {
					std::string childNodeName = node->children.data[childIndex]->name.data;
					bool childIsBone          = nameToBoneIndex.contains(childNodeName);
					if (childIsBone) {
						bones[boneIndex].children.push_back(nameToBoneIndex[childNodeName]);
					}
				}
				traverseNodeHierarchy(node->children.data[childIndex], boneIndex);
			}
		};
		traverseNodeHierarchy(m_modelScene->root_node, NO_PARENT);
		// m_skeleton->Traverse();

		size_t bufferSize = numberOfBones * sizeof(glm::mat4);  // in bytes
		skeletonUbo       = std::make_shared<Vulkan::Buffer>(bufferSize);
		skeletonUbo->Map();
	}
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