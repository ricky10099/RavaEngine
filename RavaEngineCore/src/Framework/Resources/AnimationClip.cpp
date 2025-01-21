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
		Bone& bone          = skeleton.bones[i];
		AnimNode animNodes = animNodesList[i];

		glm::quat rot   = glm::lerp(animNodes.rot[f0], animNodes.rot[f1], t);
		glm::vec3 pos   = glm::mix(animNodes.pos[f0], animNodes.pos[f1], t);
		glm::vec3 scale = glm::mix(animNodes.scale[f0], animNodes.scale[f1], t);

		bone.localTransform = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scale);

		// bone.boneOffset = glm::translate(glm::mat4(1.0f), pos) * glm::mat4(rot) * glm::scale(glm::mat4(1.0f), scale);
	}
}
}  // namespace Rava