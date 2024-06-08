#include "lmbpch.h"
#include "core/logger.h"
#include "core/typedefs.h"
#include "core/event.h"

using Limbo::Logger;
using namespace Limbo;
int main() {
	Logger& logger = Logger::GetInstance();
	logger.setLogFile("test.txt");
	logger.setLogLevel(Limbo::LogLevel::Trace);
	logger.setMaxFileSize(static_cast<size_t>(1024) * 1024);

	for (int i = 0; i < 10000; i++) {
		logger.logTrace("This is a trace message");
		logger.logDebug("This is a debug message");
		logger.logInfo("This is an info message");
		logger.logWarning("This is a warning message");
		logger.logError("This is an error message");
		logger.logCritical("This is a critical message");
	}

	std::cin.get();
	return 0;
}