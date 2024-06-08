#pragma once
#include "lmbpch.h"

namespace Limbo {
	enum class LogLevel {
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Critical,
	};

	class Logger {
	public:
		static Logger& GetInstance() {
			static Logger instance;
			return instance;
		}
		void setLogLevel(LogLevel level) {
			m_logLevel = level;
		}
		void log(LogLevel level, const std::string& message) {
			if (level < m_logLevel) {
				return;
			}
			std::lock_guard<std::mutex> lock(m_mutex);
			const char* levelString = getLogLevelString(level);
			std::cout << "[" << levelString << "]: " << message << std::endl;
			if (m_logFile.is_open()) {
				m_logFile << "[" << levelString << "]: " << message << std::endl;
			}
		}
		void log(LogLevel level, const std::string& message, const char* file, int line) {
			std::string fullMessage = "[" + std::string(file) + ":" + std::to_string(line) + "] " + message;//TODO: make this more efficient
			log(level, fullMessage);
		}
		void log(LogLevel level, const char* message) {
			log(level, std::string(message));
		}
		void logTrace(const std::string& message) {
			log(LogLevel::Trace, message);
		}
		void logTrace(const std::string& message, const char* file, int line) {
			log(LogLevel::Trace, message, file, line);
		}
		void logDebug(const std::string& message) {
			log(LogLevel::Debug, message);
		}
		void logDebug(const std::string& message, const char* file, int line) {
			log(LogLevel::Debug, message, file, line);
		}
		void logInfo(const std::string& message) {
			log(LogLevel::Info, message);
		}
		void logInfo(const std::string& message, const char* file, int line) {
			log(LogLevel::Info, message, file, line);
		}
		void logWarning(const std::string& message, const char* file, int line) {
			log(LogLevel::Warning, message, file, line);
		}
		void logWarning(const std::string& message) {
			log(LogLevel::Warning, message);
		}
		void logError(const std::string& message, const char* file, int line) {
			log(LogLevel::Error, message, file, line);
		}
		void logError(const std::string& message) {
			log(LogLevel::Error, message);
		}
		void logCritical(const std::string& message, const char* file, int line) {
			log(LogLevel::Critical, message, file, line);
		}
		void logCritical(const std::string& message) {
			log(LogLevel::Critical, message);
		}
		void setLogFile(const std::string& filename) {
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_logFile.is_open()) {
				m_logFile.close();
			}
			std::filesystem::path logFilePath = filename;
			if (!std::filesystem::exists(logFilePath)) {
				std::ofstream file(filename);
				if (!file) {
					std::cerr << "Failed to create log file: " << filename << std::endl;
					return;
				}
				file.close();
				m_logFilename = filename;
			}

			m_logFile.open(filename, std::ios::out | std::ios::app);
			if (!m_logFile) {
				std::cerr << "Failed to open log file: " << filename << std::endl;
			}
		}
		void closeLogFile() {
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_logFile.is_open()) {
				m_logFile.close();
			}
		}
		void eraseLogFile() {
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_logFile.is_open()) {
				m_logFile.close();
			}
			m_logFile.open(m_logFilename, std::ofstream::out | std::ofstream::trunc);
			if (m_logFile.is_open()) {
				std::cout << "Log file erased" << std::endl;
			}
			else {
				std::cerr << "Failed to erase log file" << std::endl;
			}
		}
		const char* getLogLevelString(LogLevel level) {
			switch (level) {
			case LogLevel::Trace: return "TRACE";
			case LogLevel::Debug: return "DEBUG";
			case LogLevel::Info: return "INFO";
			case LogLevel::Warning: return "WARNING";
			case LogLevel::Error: return "ERROR";
			case LogLevel::Critical: return "CRITICAL";
			}
		}
	private:
		Logger() {
			m_logLevel = LogLevel::Info;
		}
		~Logger() {
			m_logFile.close();
		}
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		LogLevel m_logLevel;
		std::ofstream m_logFile;
		std::string m_logFilename;
		std::mutex m_mutex;
	};



}