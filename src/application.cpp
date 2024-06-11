#include "lmbpch.h"
#include "application.h"
#include "core/event.h"
#include "core/logger.h"

#include <GLFW/glfw3.h>

namespace Limbo  {

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create());
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		while (m_Running) {
			glClearColor(1, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			m_Window -> OnUpdate();
		}
	}
	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		return false;
	}
}
