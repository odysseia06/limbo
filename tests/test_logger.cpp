#include "gtest/gtest.h"
#include "core/logger.h"

using namespace Limbo;

TEST(LoggerTest, LogToFile) {
	Logger& logger = Logger::GetInstance();
	logger.setLogLevel(LogLevel::Trace);
	logger.setLogFile("test.txt");

	logger.logTrace("This is a trace message");
	logger.logDebug("This is a debug message");
	logger.logInfo("This is an info message");
	logger.logWarning("This is a warn message");
	logger.logError("This is an error message");
	logger.logCritical("This is a critical message");

	std::ifstream logFile("test.txt");
	std::string content((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
	logFile.close();

	EXPECT_NE(content.find("This is a trace message"), std::string::npos);
	EXPECT_NE(content.find("This is a debug message"), std::string::npos);
	EXPECT_NE(content.find("This is an info message"), std::string::npos);
	EXPECT_NE(content.find("This is a warn message"), std::string::npos);
	EXPECT_NE(content.find("This is an error message"), std::string::npos);
	EXPECT_NE(content.find("This is a critical message"), std::string::npos);
}