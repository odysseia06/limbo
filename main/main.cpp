#include "core/logger.h"
#include "core/typedefs.h"
using Limbo::Logger;
int main() {
	Logger& logger = Logger::GetInstance();
	logger.setLogFile("test.txt");
	logger.setLogLevel(Limbo::LogLevel::Trace);


	LOG_TRACE("Testing the logger with a trace message");

	std::cin.get();
	return 0;
}