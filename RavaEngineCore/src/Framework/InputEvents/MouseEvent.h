#pragma once

#include "Framework/InputEvents/Event.h"
#include "Framework/InputEvents/MouseButton.h"

namespace Rava {
class MouseMovedEvent : public Event {
   public:
	MouseMovedEvent(float x, float y)
		: m_mouseX(x)
		, m_mouseY(y) {}

	inline float GetX() const { return m_mouseX; }
	inline float GetY() const { return m_mouseY; }

	EVENT_CLASS_CATEGORY(EventCategoryMouse);
	EVENT_CLASS_TYPE(MouseMoved);

	std::string ToString() const override {
		std::stringstream str;
		str << "MouseMovedEvent: MouseX: " << m_mouseX << ", MouseY: " << m_mouseY;
		return str.str();
	}

   private:
	float m_mouseX, m_mouseY;
};

class MouseScrolledEvent : public Event {
   public:
	MouseScrolledEvent(float xOffset, float yOffset)
		: m_mouseOffsetX(xOffset)
		, m_mouseOffsetY(yOffset) {}

	inline float GetX() const { return m_mouseOffsetX; }
	inline float GetY() const { return m_mouseOffsetY; }

	EVENT_CLASS_CATEGORY(EventCategoryMouse);
	EVENT_CLASS_TYPE(MouseScrolled);

	std::string ToString() const override {
		std::stringstream str;
		str << "MouseScrolledEvent: MouseOffsetX: " << m_mouseOffsetX << ", MouseOffsetY " << m_mouseOffsetY;
		return str.str();
	}

   private:
	float m_mouseOffsetX, m_mouseOffsetY;
};

class MouseButtonEvent : public Event {
   public:
	inline int GetMouseButton() const { return m_mouseButton; }

	EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryMouseButton);

   protected:
	MouseButtonEvent(MouseButton mouseButton)
		: m_mouseButton(mouseButton) {}

	MouseButton m_mouseButton;
};

class MouseButtonPressedEvent : public MouseButtonEvent {
   public:
	MouseButtonPressedEvent(MouseButton mouseButton, double posX, double posY)
		: MouseButtonEvent(mouseButton)
		, m_mouseX(posX)
		, m_mouseY(posY) {}

	inline int GetButton() const { return m_mouseButton; }
	inline double GetX() const { return m_mouseX; }
	inline double GetY() const { return m_mouseY; }

	EVENT_CLASS_TYPE(MouseButtonPressed);

	std::string ToString() const override {
		std::stringstream str;
		str << "MouseButtonPressedEvent: MouseButton: " << GetMouseButton();
		return str.str();
	}

   private:
	double m_mouseX;
	double m_mouseY;
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
   public:
	MouseButtonReleasedEvent(MouseButton mouseButton)
		: MouseButtonEvent(mouseButton) {}

	EVENT_CLASS_TYPE(MouseButtonReleased);

	std::string ToString() const override {
		std::stringstream str;
		str << "MouseButtonReleasedEvent: MouseButton: " << GetMouseButton();
		return str.str();
	}
};
}  // namespace Rava