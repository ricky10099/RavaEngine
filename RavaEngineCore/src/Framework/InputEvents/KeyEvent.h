#pragma once

#include "Framework/InputEvents/Event.h"
#include "Framework/InputEvents/KeyCode.h"

namespace Rava {
class KeyEvent : public Event {
   public:
	inline int GetKeyCode() const { return m_keyCode; }

	EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard);

   protected:
	KeyEvent(KeyCode keyCode)
		: m_keyCode(keyCode) {}

   private:
	KeyCode m_keyCode;
};

class KeyPressedEvent : public KeyEvent {
   public:
	KeyPressedEvent(KeyCode keyCode, bool isRepeat = false)
		: KeyEvent(keyCode)
		, m_isRepeat(isRepeat) {}

	EVENT_CLASS_TYPE(KeyPressed)

	std::string ToString() const override {
		std::stringstream str;
		str << "KeyPressedEvent: KeyCode: " << GetKeyCode() << " (repeat = " << m_isRepeat << ")";
		return str.str();
	}

   private:
	bool m_isRepeat;
};

class KeyReleasedEvent : public KeyEvent {
   public:
	KeyReleasedEvent(KeyCode keyCode)
		: KeyEvent(keyCode) {}

	EVENT_CLASS_TYPE(KeyReleased)

	std::string ToString() const override {
		std::stringstream str;
		str << "KeyReleasedEvent: KeyCode: " << GetKeyCode();
		return str.str();
	}
};

class KeyTypedEvent : public KeyEvent {
   public:
	KeyTypedEvent(KeyCode keycode)
		: KeyEvent(keycode) {}

	EVENT_CLASS_TYPE(KeyTyped);

	std::string ToString() const override {
		std::stringstream ss;
		ss << "KeyTypedEvent: KeyCode: " << GetKeyCode();
		return ss.str();
	}
};
}  // namespace Rava