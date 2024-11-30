#pragma once

#include "window.h"

namespace Limbo {
	class Application : public EventListener{
	public:
		Application();
		virtual ~Application();

		void Run();

		bool onEvent(Event& event) override;
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;

		EventDispatcher m_EventDispatcher;
	};

}