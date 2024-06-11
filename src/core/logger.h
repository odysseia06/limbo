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
			std::time_t now = std::time(nullptr);
			std::tm* localTime = std::localtime(&now);
			std::stringstream ss;
			ss << "[" << (localTime->tm_year + 1900) << "-" << (localTime->tm_mon + 1) << "-" << localTime->tm_mday
				<< " " << localTime->tm_hour << ":" << localTime->tm_min << ":" << localTime->tm_sec << "] "
				<< "[" << levelString << "]: " << message << std::endl;
#if defined(L_DEBUG)
			std::cout << ss.str();
#endif
			if (m_logFile.is_open()) {
				m_logFile << ss.str();
				m_logFile.flush();
				if (m_maxFileSize > 0)
					checkAndTrimLogFile();
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
		template<typename... Args> void logTrace(const std::string& format, Args... args) {
			logTrace(formatString(format, args...));
		}
		template<typename... Args> void logDebug(const std::string& format, Args... args) {
			logDebug(formatString(format, args...));
		}
		template<typename... Args> void logInfo(const std::string& format, Args... args) {
			logInfo(formatString(format, args...));
		}
		template<typename... Args> void logWarning(const std::string& format, Args... args) {
			logWarning(formatString(format, args...));
		}
		template<typename... Args> void logError(const std::string& format, Args... args) {
			logError(formatString(format, args...));
		}
		template<typename... Args> void logCritical(const std::string& format, Args... args) {
			logCritical(formatString(format, args...));
		}
		template<typename... Args> void logTrace(const std::string& format, const char* file, int line, Args... args) {
			logTrace(formatString(format, args...), file, line);
		}
		template<typename... Args> void logDebug(const std::string& format, const char* file, int line, Args... args) {
			logDebug(formatString(format, args...), file, line);
		}
		template<typename... Args> void logInfo(const std::string& format, const char* file, int line, Args... args) {
			logInfo(formatString(format, args...), file, line);
		}
		template<typename... Args> void logWarning(const std::string& format, const char* file, int line, Args... args) {
			logWarning(formatString(format, args...), file, line);
		}
		template<typename... Args> void logError(const std::string& format, const char* file, int line, Args... args) {
			logError(formatString(format, args...), file, line);
		}
		template<typename... Args> void logCritical(const std::string& format, const char* file, int line, Args... args) {
			logCritical(formatString(format, args...), file, line);
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
			default: return "UNKNOWN";
			}
		}
		void setMaxFileSize(std::size_t size) {
			m_maxFileSize = size;
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
		std::size_t m_maxFileSize = 0;

		void checkAndTrimLogFile() {
			m_logFile.seekp(0, std::ios_base::end);
			std::size_t fileSize = m_logFile.tellp();
			if (fileSize > m_maxFileSize) {
				trimLogFile();
			}
		}
		void trimLogFile() {
			m_logFile.close();
			std::ifstream inputFile(m_logFilename, std::ios::in);
			std::string content;
			std::size_t keepSize = m_maxFileSize / 2;
			inputFile.seekg(0, std::ios_base::end);
			std::size_t fileSize = inputFile.tellg();
			inputFile.seekg(0, std::ios_base::beg);
			if (fileSize > m_maxFileSize) {
				inputFile.seekg(fileSize - keepSize, std::ios_base::beg);
				std::getline(inputFile, content);//Skip the next line
			}
			std::ofstream outputFile(m_logFilename, std::ios::out | std::ios::trunc);
			while (std::getline(inputFile, content)) {
				outputFile << content << std::endl;
			}
			inputFile.close();
			outputFile.close();
			m_logFile.open(m_logFilename, std::ios::out | std::ios::app);
		}
		int parseIndex(const char*& format) {
			int index = 0;
			while (*format >= '0' && *format <= '9') {
				index = index * 10 + (*format - '0');
				++format;
			}
			return index;
		}
		void formatImpl(std::ostringstream& stream, const char* format, const std::vector<std::string>& args) {
			while (*format) {
				if (*format == '{' && *(format + 1) >= '0' && *(format + 1) <= '9') {
					++format; // Skip '{'
					int index = parseIndex(format);
					if (*format == '}') {
						if (index >= 0 && index < static_cast<int>(args.size())) {
							stream << args[index];
						}
						else {
							throw std::out_of_range("Format index out of range");
						}
						++format; // Skip '}'
					}
					else {
						throw std::runtime_error("Invalid format specifier");
					}
				}
				else if (*format == '{' && *(format + 1) == '{') {
					stream << '{';
					format += 2;
				}
				else if (*format == '}' && *(format + 1) == '}') {
					stream << '}';
					format += 2;
				}
				else {
					stream << *format++;
				}
			}
		}
		template<typename T>
		void collectArgs(std::vector<std::string>& args, T value) {
			std::ostringstream stream;
			stream << value;
			args.push_back(stream.str());
		}

		template<typename T, typename... Args>
		void collectArgs(std::vector<std::string>& args, T value, Args... rest) {
			collectArgs(args, value);
			collectArgs(args, rest...);
		}

		// Main formatting function that starts the process.
		template<typename... Args>
		std::string formatString(const std::string& format, Args... args) {
			std::vector<std::string> argList;
			collectArgs(argList, args...);
			std::ostringstream stream;
			formatImpl(stream, format.c_str(), argList);
			return stream.str();
		}
	};
}