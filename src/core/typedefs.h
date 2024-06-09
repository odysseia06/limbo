#pragma once
#include "logger.h"

#define LOG_TRACE_FULL(message)		Limbo::Logger::GetInstance().logTrace(message, __FILE__, __LINE__)
#define LOG_DEBUG_FULL(message)		Limbo::Logger::GetInstance().logDebug(message, __FILE__, __LINE__)
#define LOG_INFO_FULL(message)		Limbo::Logger::GetInstance().logInfo(message, __FILE__, __LINE__)
#define LOG_WARNING_FULL(message)	Limbo::Logger::GetInstance().logWarning(message, __FILE__, __LINE__)
#define LOG_ERROR_FULL(message)		Limbo::Logger::GetInstance().logError(message, __FILE__, __LINE__)
#define LOG_CRITICAL_FULL(message)	Limbo::Logger::GetInstance().logCritical(message, __FILE__, __LINE__)

#define LOGF_TRACE_FULL(...)		Limbo::Logger::GetInstance().logTrace(__VA_ARGS__, __FILE__, __LINE__)
#define LOGF_DEBUG_FULL(...)		Limbo::Logger::GetInstance().logDebug(__VA_ARGS__, __FILE__, __LINE__)
#define LOGF_INFO_FULL(...)		    Limbo::Logger::GetInstance().logInfo(__VA_ARGS__, __FILE__, __LINE__)
#define LOGF_WARNING_FULL(...)	    Limbo::Logger::GetInstance().logWarning(__VA_ARGS__, __FILE__, __LINE__)
#define LOGF_ERROR_FULL(...)		Limbo::Logger::GetInstance().logError(__VA_ARGS__, __FILE__, __LINE__)
#define LOGF_CRITICAL_FULL(...)	    Limbo::Logger::GetInstance().logCritical(__VA_ARGS__, __FILE__, __LINE__)

#define LOG_TRACE(message)			Limbo::Logger::GetInstance().logTrace(message)
#define LOG_DEBUG(message)			Limbo::Logger::GetInstance().logDebug(message)
#define LOG_INFO(message)			Limbo::Logger::GetInstance().logInfo(message)
#define LOG_WARNING(message)		Limbo::Logger::GetInstance().logWarning(message)
#define LOG_ERROR(message)			Limbo::Logger::GetInstance().logError(message)
#define LOG_CRITICAL(message)		Limbo::Logger::GetInstance().logCritical(message)

#define LOGF_TRACE(...)			    Limbo::Logger::GetInstance().logTrace(__VA_ARGS__)
#define LOGF_DEBUG(...)			    Limbo::Logger::GetInstance().logDebug(__VA_ARGS__)
#define LOGF_INFO(...)			    Limbo::Logger::GetInstance().logInfo(__VA_ARGS__)
#define LOGF_WARNING(...)		    Limbo::Logger::GetInstance().logWarning(__VA_ARGS__)
#define LOGF_ERROR(...)			    Limbo::Logger::GetInstance().logError(__VA_ARGS__)
#define LOGF_CRITICAL(...)		    Limbo::Logger::GetInstance().logCritical(__VA_ARGS__)

#define LOG_TRACE_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logTrace(message); }
#define LOG_DEBUG_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logDebug(message); }
#define LOG_INFO_IF(condition, message)			if (condition) { Limbo::Logger::GetInstance().logInfo(message); }
#define LOG_WARNING_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logWarning(message); }
#define LOG_ERROR_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logError(message); }
#define LOG_CRITICAL_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logCritical(message); }

#define LMB_ASSERT(condition, format, ...)                                  \
    do {                                                            \
        if (!(condition)) {                                         \
            Limbo::Logger::GetInstance().logError("Assertion failed: " #condition ", " format, ##__VA_ARGS__); \
            std::abort();                                           \
        }                                                           \
    } while (false)

#define BIT(x) (1 << x)