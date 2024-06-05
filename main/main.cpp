#include "core/logger.h"
#include "core/typedefs.h"
#include "core/event.h"

using Limbo::Logger;
using namespace Limbo;
int main() {
	Logger& logger = Logger::GetInstance();
	logger.setLogFile("test.txt");
	logger.setLogLevel(Limbo::LogLevel::Trace);


	LOG_TRACE("Testing the logger with a trace message");

	EventDispatcher dispatcher;
	ConcreteEventListener listener;

	dispatcher.registerListener(EventType::WindowClose, &listener);
	dispatcher.registerListener(EventType::WindowResize, &listener);

	WindowCloseEvent closeEvent;
	dispatcher.dispatch(closeEvent);

	WindowResizeEvent resizeEvent(1280, 720);
	dispatcher.dispatch(resizeEvent);

	std::cin.get();
	return 0;
}