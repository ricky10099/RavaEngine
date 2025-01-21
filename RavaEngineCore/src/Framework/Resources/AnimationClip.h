#pragma once

#include "Framework/Timestep.h"
#include "Framework/Resources/Skeleton.h"
#include "Framework/Resources/MeshModel.h"

namespace Rava {
struct Skeleton;
struct Bone;
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

	// struct Channel {
	//	Path path;
	//	int samplerIndex;
	//	int node;
	// };

	// struct Sampler {
	//	std::vector<float> timestamps;
	//	std::vector<glm::vec4> valuesToInterpolate;
	//	InterpolationMethod interpolation;
	// };

	struct Frame {
		double time;
	};

	struct Position : public Frame {
		glm::vec3 xyz;
	};

	struct Rotation : public Frame {
		glm::quat quaternion;
	};

	struct Scaling : public Frame {
		glm::vec3 xyz;
	};

	struct Keyframe {
		float time;
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
	};

	struct BoneAnimation {
		std::vector<Keyframe> keyframes;
	};

	/// a frame of animation for all of the bones.
	struct AnimNode {
		// Node modelNode;
		// std::vector<Position> positions;
		// std::vector<Rotation> rotations;
		// std::vector<Scaling> scales;

		float timeBegin;
		float framerate;
		size_t frameCount;
		std::vector<glm::quat> rot;
		std::vector<glm::vec3> pos;
		std::vector<glm::vec3> scale;
	};

   public:
	// std::vector<Sampler> samplers;
	// std::vector<Channel> channels;
	// std::vector<Node> nodes;
	std::vector<AnimNode> animNodesList;
	std::map<std::string, BoneAnimation> boneAnimations;

   public:
	AnimationClip(std::string_view name);

	void Start() { m_currentKeyFrameTime = m_firstKeyFrameTime; }
	void Stop() { m_currentKeyFrameTime = m_lastKeyFrameTime + 1.0f; }

	void Update(Skeleton& skeleton);

	void SetRepeat(bool repeat) { m_repeat = repeat; }
	void SetFirstKeyFrameTime(float firstKeyFrameTime) { m_firstKeyFrameTime = firstKeyFrameTime; }
	void SetLastKeyFrameTime(float lastKeyFrameTime) { m_lastKeyFrameTime = lastKeyFrameTime; }
	void SetFramerate(float framerate) { m_framerate = framerate; }
	void SetTotalFrameCount(u32 totalFrameCount) { m_totalFrameCount = totalFrameCount; }

	bool IsRunning() const { return (m_repeat || (m_currentKeyFrameTime <= m_lastKeyFrameTime)); }
	bool WillExpire() const { return (!m_repeat && ((m_currentKeyFrameTime + Timestep::Count()) > m_lastKeyFrameTime)); }
	std::string_view GetName() const { return m_name; }
	float GetDuration() const { return m_lastKeyFrameTime - m_firstKeyFrameTime; }
	float GetFramerates() const { return m_framerate; }
	float GetCurrentFrameTime() const { return m_currentKeyFrameTime - m_firstKeyFrameTime; }
	u32 GetTotalFrameTime() const { return m_totalFrameCount; }

   private:
	std::string_view m_name;

	bool m_repeat = false;

	// relative animation time
	float m_firstKeyFrameTime   = 0.0f;
	float m_lastKeyFrameTime    = 0.0f;
	float m_framerate           = 0.0f;
	float m_currentKeyFrameTime = 0.0f;
	u32 m_totalFrameCount       = 0;
};
}  // namespace Rava