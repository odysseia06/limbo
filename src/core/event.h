#pragma once
#include "core/typedefs.h"
#include <unordered_map>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

namespace Limbo {
	enum class EventType {
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};
	enum class EventCategory {
		None = 0,
		Application = BIT(0),
		Input = BIT(1),
		Keyboard = BIT(2),
		Mouse = BIT(3),
		MouseButton = BIT(4),
	};
	inline EventCategory operator|(EventCategory a, EventCategory b) {
		return static_cast<EventCategory>(static_cast<int>(a) | static_cast<int>(b));
	}
	inline int operator&(EventCategory a, EventCategory b) {
		return static_cast<int>(a) & static_cast<int>(b);
	}
	inline std::ostream& operator<<(std::ostream& os, const EventCategory& category) {
		switch (category) {
		case EventCategory::None: os << "None"; break;
		case EventCategory::Application: os << "Application"; break;
		case EventCategory::Input: os << "Input"; break;
		case EventCategory::Keyboard: os << "Keyboard"; break;
		case EventCategory::Mouse: os << "Mouse"; break;
		case EventCategory::MouseButton: os << "MouseButton"; break;
		}
		return os;
	}
	inline std::ostream& operator<<(std::ostream& os, const EventType& type) {
		switch (type) {
		case EventType::None: os << "None"; break;
		case EventType::WindowClose: os << "WindowClose"; break;
		case EventType::WindowResize: os << "WindowResize"; break;
		case EventType::WindowFocus: os << "WindowFocus"; break;
		case EventType::WindowLostFocus: os << "WindowLostFocus"; break;
		case EventType::WindowMoved: os << "WindowMoved"; break;
		case EventType::AppTick: os << "AppTick"; break;
		case EventType::AppUpdate: os << "AppUpdate"; break;
		case EventType::AppRender: os << "AppRender"; break;
		case EventType::KeyPressed: os << "KeyPressed"; break;
		case EventType::KeyReleased: os << "KeyReleased"; break;
		case EventType::KeyTyped: os << "KeyTyped"; break;
		case EventType::MouseButtonPressed: os << "MouseButtonPressed"; break;
		case EventType::MouseButtonReleased: os << "MouseButtonReleased"; break;
		case EventType::MouseMoved: os << "MouseMoved"; break;
		case EventType::MouseScrolled: os << "MouseScrolled"; break;
		}
		return os;
	}
#define EVENT_CLASS_TYPE(type) static EventType getStaticType() { return EventType::type; } \
								virtual EventType getEventType() const override { return getStaticType(); } \
								virtual const char* getEventName() const override { return #type; }
#define EVENT_CLASS_CATEGORY(category) virtual EventCategory getCategoryFlags() const override { return category; }
	class Event {
		friend class EventDispatcher;
	public:
		virtual ~Event() = default;
		virtual EventType getEventType() const = 0;
		virtual const char* getEventName() const = 0;
		virtual std::string toString() const { return getEventName(); }
		virtual EventCategory getCategoryFlags() const = 0;
		inline bool isInCategory(EventCategory category) const {
			return getCategoryFlags() & category;
		}
		inline bool isHandled() const { return m_handled; }
	protected:
		bool m_handled = false;
	};
	class WindowCloseEvent : public Event {
	public:
		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategory::Application)
		std::string toString() const override { return getEventName(); }
	};
	class WindowResizeEvent : public Event {
	public:
		WindowResizeEvent(unsigned int width, unsigned int height) : m_width(width), m_height(height) {}
		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategory::Application)
		std::string toString() const override {
			std::stringstream ss;
			ss << "WindowResizeEvent Width: " << m_width << ", " << "Height: " << m_height;
			return ss.str();
		}
		unsigned int getWidth() const { return m_width; }
		unsigned int getHeight() const { return m_height; }
	private:
		unsigned int m_width, m_height;
	};
	class WindowFocusEvent : public Event {
	public:
		EVENT_CLASS_TYPE(WindowFocus)
		EVENT_CLASS_CATEGORY(EventCategory::Application)
		std::string toString() const override { return getEventName(); }
	};
	class WindowLostFocusEvent : public Event {
	public:
		EVENT_CLASS_TYPE(WindowLostFocus)
		EVENT_CLASS_CATEGORY(EventCategory::Application)
		std::string toString() const override { return getEventName(); }
	};
	class WindowMovedEvent : public Event {
	public:
		EVENT_CLASS_TYPE(WindowMoved)
		EVENT_CLASS_CATEGORY(EventCategory::Application)
		std::string toString() const override { return getEventName(); }
	};
	class AppTickEvent : public Event {
	public:
		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategory::Application)
		std::string toString() const override { return getEventName(); }
	};
	class AppUpdateEvent : public Event {
	public:
		EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategory::Application)
		std::string toString() const override { return getEventName(); }
	};
	class AppRenderEvent : public Event {
	public:
		EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategory::Application)
		std::string toString() const override { return getEventName(); }
	};
	class KeyEvent : public Event {
	public:
		inline int getKeyCode() const { return m_keyCode; }
		EVENT_CLASS_CATEGORY(EventCategory::Keyboard | EventCategory::Input)
	protected:
		KeyEvent(int keyCode) : m_keyCode(keyCode) {}
		int m_keyCode;
	};
	class KeyPressedEvent : public KeyEvent {
	public:
		KeyPressedEvent(int keyCode, int repeatCount) : KeyEvent(keyCode), m_repeatCount(repeatCount) {}
		inline int getRepeatCount() const { return m_repeatCount; }
		EVENT_CLASS_TYPE(KeyPressed)
		std::string toString() const override {
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_keyCode <<" (" << m_repeatCount << " repeats)";
			return ss.str();
			}
	private:
		int m_repeatCount;
	};
	class KeyReleasedEvent : public KeyEvent {
	public:
		KeyReleasedEvent(int keyCode) : KeyEvent(keyCode) {}
		EVENT_CLASS_TYPE(KeyReleased)
		std::string toString() const override {
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_keyCode;
			return ss.str();
		}
	};
	class KeyTypedEvent : public KeyEvent {
	public:
		KeyTypedEvent(int keyCode) : KeyEvent(keyCode) {}
		EVENT_CLASS_TYPE(KeyTyped)
		std::string toString() const override {
			std::stringstream ss;
			ss << "KeyTypedEvent: " << m_keyCode;
			return ss.str();
		}
	};
	class MouseButtonEvent : public Event {
	public:
		inline int getButton() const { return m_button; }
		EVENT_CLASS_CATEGORY(EventCategory::MouseButton | EventCategory::Input)
	protected:
		MouseButtonEvent(int button) : m_button(button) {}
		int m_button;
	};
	class MouseButtonPressedEvent : public MouseButtonEvent {
	public:
		MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}
		EVENT_CLASS_TYPE(MouseButtonPressed)
		std::string toString() const override {
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_button;
			return ss.str();
		}
	};
	class MouseButtonReleasedEvent : public MouseButtonEvent {
	public:
		MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}
		EVENT_CLASS_TYPE(MouseButtonReleased)
		std::string toString() const override {
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_button;
			return ss.str();
		}
	};
	class MouseMovedEvent : public Event {
	public:
		MouseMovedEvent(float x, float y) : m_x(x), m_y(y) {}
		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input)
		std::string toString() const override {
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_x << ", " << m_y;
			return ss.str();
		}
		inline float getX() const { return m_x; }
		inline float getY() const { return m_y; }
	private:
		float m_x, m_y;
	};
	class MouseScrolledEvent : public Event {
	public:
		MouseScrolledEvent(float xOffset, float yOffset) : m_xOffset(xOffset), m_yOffset(yOffset) {}
		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input)
		std::string toString() const override {
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << m_xOffset << ", " << m_yOffset;
			return ss.str();
		}
		inline float getXOffset() const { return m_xOffset; }
		inline float getYOffset() const { return m_yOffset; }
	private:
		float m_xOffset, m_yOffset;
	};

	class EventListener {
		friend class EventDispatcher;
	public:
		virtual ~EventListener() = default;
		virtual bool onEvent(Event& event) = 0;
	};
	class EventDispatcher {
	public:
		void registerListener(EventType type, EventListener* listener) {
			m_listeners[type].push_back(listener);
		}
		void unregisterListener(EventType type, EventListener* listener) {
			auto& listeners = m_listeners[type];
			listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
		}
		void dispatch(Event& event, bool stopOnHandled = true) {
			auto it = m_listeners.find(event.getEventType());
			if (it != m_listeners.end()) {
				for (auto listener : it->second) {
					event.m_handled = listener->onEvent(event);
					if (event.isHandled() && stopOnHandled) {
						LOG_TRACE("Event handled: " + event.toString());
						break;
					}
				}
				if (!stopOnHandled && event.isHandled()) {
					LOG_TRACE("Event handled: " + event.toString());
				}
			}
		}
	private:
		std::unordered_map<EventType, std::vector<EventListener*>> m_listeners;
	};
	class ConcreteEventListener : public EventListener {
	public:
		bool onEvent(Event& event) override {
			LOG_TRACE("Event received: " + event.toString());
			if (event.getEventType() == EventType::WindowClose) {
				LOG_TRACE("Window close event received");
			}
			else if (event.getEventType() == EventType::WindowResize) {
				auto& resizeEvent = static_cast<WindowResizeEvent&>(event);
				LOG_TRACE("Window resize event received: " + std::to_string(resizeEvent.getWidth()) + ", " + std::to_string(resizeEvent.getHeight()));
			}
			return true;
		}
	};
	inline std::ostream& operator<<(std::ostream& os, const Event& event) {
		os << event.toString();
		return os;
	}
}