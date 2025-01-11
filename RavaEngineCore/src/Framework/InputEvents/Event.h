#pragma once

namespace Rava {
enum class EventType {
	None = 0,
	WindowClose,
	WindowResize,
	KeyPressed,
	KeyReleased,
	KeyTyped,
	MouseButtonPressed,
	MouseButtonReleased,
	MouseMoved,
	MouseScrolled,
	ControllerButtonPressed,
	ControllerButtonReleased,
	ControllerAxisMoved,
	JoystickButtonPressed,
	JoystickButtonReleased,
	JoystickAxisMoved,
	JoystickHatMoved,
	JoystickBallMoved,
	TimerExpired,
	ApplicationEvent,
};

enum EventCategory {
	None                          = 0,
	EventCategoryApplication      = BIT(0),
	EventCategoryInput            = BIT(1),
	EventCategoryKeyboard         = BIT(2),
	EventCategoryMouse            = BIT(3),
	EventCategoryMouseButton      = BIT(4),
	EventCategoryController       = BIT(5),
	EventCategoryControllerButton = BIT(6),
	EventCategoryJoystick         = BIT(7),
	EventCategoryJoystickButton   = BIT(8),
	EventCategoryTimer            = BIT(9),
};

#define EVENT_CLASS_CATEGORY(x)             \
	int GetCategoryFlags() const override { \
		return x;                           \
	}
#define EVENT_CLASS_TYPE(x)                   \
	static EventType GetStaticType() {        \
		return EventType::x;                  \
	}                                         \
	EventType GetEventType() const override { \
		return GetStaticType();               \
	}                                         \
	const char* GetName() const override {    \
		return #x "Event";                    \
	}

class Event {
	friend class EventDispatcher;

   public:
	virtual EventType GetEventType() const = 0;
	virtual const char* GetName() const    = 0;
	virtual int GetCategoryFlags() const   = 0;
	virtual std::string ToString() const   = 0;

	inline bool IsInCategory(EventCategory category) const { return GetCategoryFlags() & category; }

	inline bool IsHandled() const { return m_handled; }

	inline void MarkAsHandled() { m_handled = true; }

   protected:
	bool m_handled = false;
};

class EventDispatcher {
	template <typename T>
	using EventFunction = std::function<bool(T&)>;

   public:
	EventDispatcher(Event& event)
		: m_event(event) {}

	template <typename T>
	bool Dispatch(EventFunction<T> func) {
		if (m_event.GetEventType() == T::GetStaticType()) {
			m_event.m_handled |= func(*(T*)&m_event);
			return true;
		}
		return false;
	}

   private:
	Event& m_event;
};

using EventCallbackFunction =  std::function<void(Event&)>;

inline std::ostream& operator<<(std::ostream& os, const Event& e) {
	return os << e.ToString();
}
}  // namespace Rava