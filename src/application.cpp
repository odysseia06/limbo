#include "lmbpch.h"
#include "application.h"
#include "core/event.h"
#include "core/logger.h"

#include <GLFW/glfw3.h>

namespace Limbo  {

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create());

		// Set the event callback to dispatch events via the EventDispatcher
		m_Window->SetEventCallback([this](Event& event) {
			m_EventDispatcher.dispatch(event);
			});

		// Register the Application as a listener for specific event types
		m_EventDispatcher.registerListener(EventType::WindowClose, this);
		m_EventDispatcher.registerListener(EventType::WindowResize, this);
	}

	Application::~Application()
	{
		// Unregister listeners if necessary
		m_EventDispatcher.unregisterListener(EventType::WindowClose, this);
		m_EventDispatcher.unregisterListener(EventType::WindowResize, this);
	}

	void Application::Run()
	{
		while (m_Running) {
			glClearColor(1, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			m_Window -> OnUpdate();
		}
	}

	bool Application::onEvent(Event& event)
	{
		switch (event.getEventType()) {
		case EventType::WindowClose:
			return OnWindowClose(static_cast<WindowCloseEvent&>(event));
		case EventType::WindowResize:
			return OnWindowResize(static_cast<WindowResizeEvent&>(event));
		default:
			return false; // Event not handled
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		LOG_TRACE("Window close event received");
		m_Running = false;
		return true; // Event handled
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		LOG_TRACE("Window resize event received: Width = " +
			std::to_string(e.getWidth()) + ", Height = " +
			std::to_string(e.getHeight()));
		return false; // Return true if you want to stop event propagation
	}
}
