#include "lmbpch.h"
#include "core/logger.h"
#include "core/typedefs.h"
#include "core/event.h"
#include "window.h"
#include "application.h"

using Limbo::Logger;
using namespace Limbo;
int main() {
	Logger& logger = Logger::GetInstance();
	logger.setLogFile("test.txt");
	logger.setLogLevel(Limbo::LogLevel::Trace);
	logger.setMaxFileSize(static_cast<size_t>(1024) * 1024);

	LOGF_INFO("Creating window {0} ({1}, {2})", 400, 800, 600);

	auto app = new Application();
	app -> Run();
	delete app;
	std::cin.get();
	return 0;
}