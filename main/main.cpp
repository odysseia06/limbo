#include "core/logger.h"
#include "core/typedefs.h"
#include "core/event.h"

using Limbo::Logger;
using namespace Limbo;
int main() {
	Logger& logger = Logger::GetInstance();
	logger.setLogFile("test.txt");
	logger.setLogLevel(Limbo::LogLevel::Trace);

	EventDispatcher dispatcher;
	ConcreteEventListener listener1;
	ConcreteEventListener listener2;

	dispatcher.registerListener(EventType::KeyPressed, &listener1);
	dispatcher.registerListener(EventType::KeyPressed, &listener2);

	KeyPressedEvent event(0, 0);
	dispatcher.dispatch(event, true);

	std::cin.get();

	logger.eraseLogFile();
	return 0;
}