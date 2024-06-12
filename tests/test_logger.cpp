#include "gtest/gtest.h"
#include "core/logger.h"

using namespace Limbo;

class LoggerTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Clear the log file before each test
		std::ofstream ofs("log.txt", std::ofstream::out | std::ofstream::trunc);
		ofs.close();
	}

	std::string readLogFile() {
		std::ifstream ifs("log.txt");
		std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
		return content;
	}
};

TEST_F(LoggerTest, LogToFile) {
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
TEST_F(LoggerTest, LogInfo) {
	Logger::GetInstance().setLogFile("log.txt");
	Logger::GetInstance().logInfo("Hello, {0}!", "World");
	std::string logContent = readLogFile();
	EXPECT_NE(logContent.find("[INFO]: Hello, World!"), std::string::npos);
}

TEST_F(LoggerTest, LogError) {
	Logger::GetInstance().setLogFile("log.txt");
	Logger::GetInstance().logError("Error: {0} + {1} != {2}", 1, 1, 3);
	std::string logContent = readLogFile();
	EXPECT_NE(logContent.find("[ERROR]: Error: 1 + 1 != 3"), std::string::npos);
}
/*
TEST_F(LoggerTest, LogWithMultipleArgs) {
	Logger::GetInstance().setLogFile("log.txt");
	Logger::GetInstance().logInfo("Creating window {0} ({1}, {2})", "MyWindow", 800, 600); //TODO: DOESNT WORK WITH DIFFERENT KINDS OF ARGS
	std::string logContent = readLogFile();
	EXPECT_NE(logContent.find("[INFO]: Creating window MyWindow (800, 600)"), std::string::npos);
}
*/
TEST_F(LoggerTest, LogInvalidFormatSpecifiers) {
	Logger::GetInstance().setLogFile("log.txt");
	EXPECT_THROW(Logger::GetInstance().logInfo("{0", "missing closing brace"), std::runtime_error);
	//EXPECT_THROW(Logger::GetInstance().logInfo("0}", "missing opening brace"), std::runtime_error);
	//EXPECT_THROW(Logger::GetInstance().logInfo("{0}{", "missing closing brace"), std::runtime_error);
	//EXPECT_THROW(Logger::GetInstance().logInfo("{", "single opening brace"), std::runtime_error);
	//EXPECT_THROW(Logger::GetInstance().logInfo("}", "single closing brace"), std::runtime_error);
}
TEST_F(LoggerTest, LogOutOfOrderPlaceholders) {
	Logger::GetInstance().setLogFile("log.txt");
	Logger::GetInstance().logInfo("{2}, {0}, and {1}", "first", "second", "third");
	std::string logContent = readLogFile();
	EXPECT_NE(logContent.find("[INFO]: third, first, and second"), std::string::npos);
}
TEST_F(LoggerTest, LogMultipleOccurrences) {
	Logger::GetInstance().setLogFile("log.txt");
	Logger::GetInstance().logInfo("{0} {0} {0}", "repeat");
	std::string logContent = readLogFile();
	EXPECT_NE(logContent.find("[INFO]: repeat repeat repeat"), std::string::npos);
}