#pragma once

#include "Framework/Timestep.h"

namespace Rava {
struct Skeleton;
class Timestep;
class AnimationClip {
   public:
	enum class Path {
		TRANSLATION,
		ROTATION,
		SCALE
	};

	enum class InterpolationMethod {
		LINEAR,
		STEP,
		CUBICSPLINE
	};

	struct Channel {
		Path path;
		int samplerIndex;
		int node;
	};

	struct Sampler {
		std::vector<float> timestamps;
		std::vector<glm::vec4> valuesToInterpolate;
		InterpolationMethod interpolation;
	};

   public:
	std::vector<Sampler> samplers;
	std::vector<Channel> channels;

   public:
	AnimationClip(std::string_view name);

	void Start() { m_currentKeyFrameTime = m_firstKeyFrameTime; }
	void Stop() { m_currentKeyFrameTime = m_lastKeyFrameTime + 1.0f; }

	void Update(Skeleton& skeleton);

	void SetRepeat(bool repeat) { m_repeat = repeat; }
	void SetFirstKeyFrameTime(float firstKeyFrameTime) { m_firstKeyFrameTime = firstKeyFrameTime; }
	void SetLastKeyFrameTime(float lastKeyFrameTime) { m_lastKeyFrameTime = lastKeyFrameTime; }

	bool IsRunning() const { return (m_repeat || (m_currentKeyFrameTime <= m_lastKeyFrameTime)); }
	bool WillExpire() const { return (!m_repeat && ((m_currentKeyFrameTime + Timestep::Count()) > m_lastKeyFrameTime)); }
	std::string_view GetName() const { return m_name; }
	float GetDuration() const { return m_lastKeyFrameTime - m_firstKeyFrameTime; }
	float GetCurrentFrameTime() const { return m_currentKeyFrameTime - m_firstKeyFrameTime; }

   private:
	std::string_view m_name;

	bool m_repeat = false;

	// relative animation time
	float m_firstKeyFrameTime   = 0.0f;
	float m_lastKeyFrameTime    = 0.0f;
	float m_currentKeyFrameTime = 0.0f;
};
}  // namespace Rava