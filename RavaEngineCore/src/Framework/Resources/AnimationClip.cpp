#include "ravapch.h"

#include "Framework/Resources/AnimationClip.h"
#include "Framework/Resources/Skeleton.h"

namespace Rava {
AnimationClip::AnimationClip(std::string_view name)
	: m_name(name) {}

void AnimationClip::Update(Skeleton& skeleton) {
	if (!IsRunning()) {
		//ENGINE_TRACE("Animation '{0}' expired", m_name);
		return;
	}

	m_currentKeyFrameTime += Timestep::Count();

	if (m_repeat && (m_currentKeyFrameTime > m_lastKeyFrameTime)) {
		m_currentKeyFrameTime = m_firstKeyFrameTime;
	}

	for (auto& channel : channels) {
		auto& sampler  = samplers[channel.samplerIndex];
		int jointIndex = skeleton.globalNodeToJointIndex[channel.node];
		auto& joint    = skeleton.joints[jointIndex];

		for (size_t i = 0; i < sampler.timestamps.size() - 1; ++i) {
			if ((m_currentKeyFrameTime >= sampler.timestamps[i]) && (m_currentKeyFrameTime <= sampler.timestamps[i + 1])) {
				switch (sampler.interpolation) {
					case InterpolationMethod::LINEAR: {
						float a =
							(m_currentKeyFrameTime - sampler.timestamps[i]) / (sampler.timestamps[i + 1] - sampler.timestamps[i]);
						switch (channel.path) {
							case Path::TRANSLATION: {
								joint.deformedNodeTranslation =
									glm::mix(sampler.valuesToInterpolate[i], sampler.valuesToInterpolate[i + 1], a);

								break;
							}
							case Path::ROTATION: {
								glm::quat quaternion1{};
								quaternion1.x = sampler.valuesToInterpolate[i].x;
								quaternion1.y = sampler.valuesToInterpolate[i].y;
								quaternion1.z = sampler.valuesToInterpolate[i].z;

								glm::quat quaternion2{};
								quaternion2.x = sampler.valuesToInterpolate[i + 1].x;
								quaternion2.y = sampler.valuesToInterpolate[i + 1].y;
								quaternion2.z = sampler.valuesToInterpolate[i + 1].z;

								joint.deformedNodeRotation = glm::normalize(glm::slerp(quaternion1, quaternion2, a));

								break;
							}
							case Path::SCALE: {
								joint.deformedNodeScale =
									glm::mix(sampler.valuesToInterpolate[i], sampler.valuesToInterpolate[i + 1], a);

								break;
							}
							default:
								ENGINE_ERROR("Path not found");
						}

						break;
					}
					case InterpolationMethod::STEP: {
						switch (channel.path) {
							case Path::TRANSLATION: {
								joint.deformedNodeTranslation = glm::vec3(sampler.valuesToInterpolate[i]);

								break;
							}
							case Path::ROTATION: {
								joint.deformedNodeRotation.x = sampler.valuesToInterpolate[i].x;
								joint.deformedNodeRotation.y = sampler.valuesToInterpolate[i].y;
								joint.deformedNodeRotation.z = sampler.valuesToInterpolate[i].z;
								joint.deformedNodeRotation.w = sampler.valuesToInterpolate[i].w;

								break;
							}
							case Path::SCALE: {
								joint.deformedNodeScale = glm::vec3(sampler.valuesToInterpolate[i]);

								break;
							}
							default:
								ENGINE_ERROR("Path not found");
						}

						break;
					}
					case InterpolationMethod::CUBICSPLINE: {
						ENGINE_WARN("Animation::Update(): Interploation method CUBICSPLINE not supported!");
						break;
					}
					default:
						ENGINE_WARN("Animation::Update(): Interploation method not supported");
						break;
				}
			}
		}
	}
}
}  // namespace Rava