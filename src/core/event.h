#pragma once
#include "core/typedefs.h"
#include <unordered_map>
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
		static EventType getStaticType() { return EventType::WindowClose; }
		EventType getEventType() const override { return getStaticType(); }
		const char* getEventName() const override { return "WindowCloseEvent"; }
		std::string toString() const override { return getEventName(); }
		EventCategory getCategoryFlags() const override { return EventCategory::Application; }
	};
	class WindowResizeEvent : public Event {
	public:
		WindowResizeEvent(unsigned int width, unsigned int height) : m_width(width), m_height(height) {}
		static EventType getStaticType() { return EventType::WindowResize; }
		EventType getEventType() const override { return getStaticType(); }
		const char* getEventName() const override { return "WindowResizeEvent"; }
		std::string toString() const override { return getEventName(); }
		EventCategory getCategoryFlags() const override { return EventCategory::Application; }
		unsigned int getWidth() const { return m_width; }
		unsigned int getHeight() const { return m_height; }
	private:
		unsigned int m_width, m_height;
	};

	class EventListener {
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
		void dispatch(Event& event) {
			auto it = m_listeners.find(event.getEventType());
			if (it != m_listeners.end()) {
				for (auto listener : it->second) {
					event.m_handled = listener->onEvent(event);
					if (event.isHandled()) {
						break;
					}
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
}