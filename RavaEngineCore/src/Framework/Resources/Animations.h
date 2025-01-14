#pragma once

#include "Framework/Resources/AnimationClip.h"

namespace Rava {
class AnimationClip;
struct Skeleton;
class Animations {
	friend class Editor;
   public:
	// used for range-based loops to traverse the array elements in m_AnimationsVector
	struct Iterator {
		// Iterator points to an array element of m_animationsVector
		Iterator(Shared<AnimationClip>* pointer) { m_pointer = pointer; }
		Iterator& operator++();                                                          // pre increment operator (next element)
		bool operator!=(const Iterator& other) { return m_pointer != other.m_pointer; }  // unequal operator
		AnimationClip& operator*() { return *(*m_pointer); }                             // dereference operator

	   private:
		Shared<AnimationClip>* m_pointer;
	};

   public:
	Iterator begin() { return Iterator(&(*m_animationsVector.begin())); }
	Iterator end() { return Iterator(&(*m_animationsVector.end())); }
	AnimationClip& operator[](std::string_view animation);
	AnimationClip& operator[](u32 index);
	size_t Size() const { return m_animations.size(); }
	void Push(const Shared<AnimationClip>& animation);

   public:

	static Unique<Animations> LoadAnimationsFromFile(std::string_view filePath);

	void Start(std::string_view animation);  // by name
	void Start(size_t index);                // by index
	void Start() { Start(0); };              // start animation 0
	void Stop();

	void Update(Skeleton& skeleton, u32 frameCounter);

	void SetRepeat(bool repeat);
	void SetRepeatAll(bool repeat);

	bool IsRunning() const;
	bool WillExpire() const;
	float GetDuration(std::string_view animation) { return m_animations[animation]->GetDuration(); }
	float GetCurrentFrameTime();
	std::string_view GetName();
	std::string GetName(u32 index) { return m_animationNames[index]; }
	int GetIndex(std::string_view animation);

   private:
	std::map<std::string_view, std::shared_ptr<AnimationClip>> m_animations;
	std::vector<std::shared_ptr<AnimationClip>> m_animationsVector;
	std::vector<std::string> m_animationNames;
	AnimationClip* m_currentAnimation = nullptr;
	u32 m_frameCounter                = 1;
	std::map<std::string_view, int> m_nameToIndex;
};
}  // namespace Rava