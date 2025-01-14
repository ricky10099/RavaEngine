#include "ravapch.h"

#include "Framework/Resources/Animations.h"
#include "Framework/Resources/AnimationClip.h"
#include "Framework/Resources/Skeleton.h"

namespace Rava {
Unique<Animations> Animations::LoadAnimationsFromFile(std::string_view filePath) {

}

AnimationClip& Animations::operator[](std::string_view animation) {
	return *m_animations[animation];
}

AnimationClip& Animations::operator[](u32 index) {
	return *m_animationsVector[index];
}

void Animations::Push(const Shared<AnimationClip>& animation) {
	if (animation) {
		m_animations[animation->GetName()] = animation;
		m_animationsVector.push_back(animation);
		m_nameToIndex[animation->GetName()] = static_cast<int>(m_animationsVector.size() - 1);
	} else {
		ENGINE_ERROR("Animations::Push: Animation is empty!");
	}
}

void Animations::Start(std::string_view animation) {
	AnimationClip* currentAnimation = m_animations[animation].get();

	if (m_currentAnimation) {
		m_currentAnimation = currentAnimation;
		m_currentAnimation->Start();
	}
}

void Animations::Start(size_t index) {
	if (!(index < m_animationsVector.size())) {
		ENGINE_ERROR("Animations::Start(u32 index) out of bounds!");
		return;
	}

	AnimationClip* currentAnimation = m_animationsVector[index].get();
	if (currentAnimation) {
		m_currentAnimation = currentAnimation;
		m_currentAnimation->Start();
	}
}

void Animations::Stop() {
	if (m_currentAnimation) {
		m_currentAnimation->Stop();
	}
}

void Animations::Update(Skeleton& skeleton, u32 frameCounter) {
	if (m_frameCounter != frameCounter) {
		m_frameCounter = frameCounter;

		if (m_currentAnimation) {
			m_currentAnimation->Update(skeleton);
		}
	}
}

void Animations::SetRepeat(bool repeat) {
	if (m_currentAnimation) {
		m_currentAnimation->SetRepeat(repeat);
	}
}

void Animations::SetRepeatAll(bool repeat) {
	for (auto& animation : m_animationsVector) {
		animation->SetRepeat(repeat);
	}
}

bool Animations::IsRunning() const {
	if (m_currentAnimation) {
		return m_currentAnimation->IsRunning();
	} else {
		return false;
	}
}

bool Animations::WillExpire() const {
	if (m_currentAnimation) {
		return m_currentAnimation->WillExpire();
	} else {
		return false;
	}
}

float Animations::GetCurrentFrameTime() {
	if (m_currentAnimation) {
		return m_currentAnimation->GetCurrentFrameTime();
	} else {
		return 0.0f;
	}
}

std::string_view Animations::GetName() {
	if (m_currentAnimation) {
		return m_currentAnimation->GetName();
	} else {
		return std::string("");
	}
}

int Animations::GetIndex(std::string_view animation) {
	bool found = false;
	for (auto& element : m_animationsVector) {
		if (element->GetName() == animation) {
			found = true;
			break;
		}
	}

	if (found) {
		return m_nameToIndex[animation];
	} else {
		return -1;
	}
}

Animations::Iterator& Animations::Iterator::operator++() {
	++m_pointer;
	return *this;
}
}  // namespace Rava