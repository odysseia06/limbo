#pragma once

#include "window.h"

namespace Limbo {
	class Application {
	public:
		Application();
		virtual ~Application();

		void Run();
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};

}