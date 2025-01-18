#include "ravapch.h"

#include "Framework/Resources/AnimationClip.h"

namespace Rava {
AnimationClip::AnimationClip(std::string_view name)
	: m_name(name) {}

void AnimationClip::Update(Skeleton& skeleton) {
	if (!IsRunning()) {
		return;
	}

	m_currentKeyFrameTime += Timestep::Count();

	if (m_repeat && (m_currentKeyFrameTime > m_lastKeyFrameTime)) {
		m_currentKeyFrameTime = m_firstKeyFrameTime;
	}

	float frame_time = (m_currentKeyFrameTime - m_firstKeyFrameTime) * 30;
	u32 f0           = glm::min((u32)frame_time + 0, m_totalFrameCount - 1);
	u32 f1           = glm::min((u32)frame_time + 1, m_totalFrameCount - 1);
	float t          = glm::min(frame_time - (float)f0, 1.0f);

	for (u32 i = 0; i < skeleton.bones.size(); ++i) {
		Bone bone           = skeleton.bones[i];
		AnimNodes animNodes = animNodesList[i];

		glm::quat rot =
			&animNodes.rotations
				? glm::lerp(animNodes.rotations[f0].quaternion, animNodes.rotations[f1].quaternion, m_currentKeyFrameTime)
				: animNodes.rotations[0].quaternion;
		glm::vec3 pos   = &animNodes.positions
							? glm::mix(animNodes.positions[f0].xyz, animNodes.positions[f1].xyz, m_currentKeyFrameTime)
							: animNodes.positions[0].xyz;
		glm::vec3 scale = &animNodes.scales ? glm::mix(animNodes.scales[f0].xyz, animNodes.scales[f1].xyz, m_currentKeyFrameTime)
											: animNodes.scales[0].xyz;

		bone.boneOffset = glm::translate(glm::mat4(1.0f), pos) * glm::mat4(rot) * glm::scale(glm::mat4(1.0f), scale);
	}
}
}  // namespace Rava