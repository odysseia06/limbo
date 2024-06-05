#pragma once
#include "logger.h"

#define LOG_TRACE_FULL(message)		Limbo::Logger::GetInstance().logTrace(message, __FILE__, __LINE__)
#define LOG_DEBUG_FULL(message)		Limbo::Logger::GetInstance().logDebug(message, __FILE__, __LINE__)
#define LOG_INFO_FULL(message)		Limbo::Logger::GetInstance().logInfo(message, __FILE__, __LINE__)
#define LOG_WARNING_FULL(message)	Limbo::Logger::GetInstance().logWarning(message, __FILE__, __LINE__)
#define LOG_ERROR_FULL(message)		Limbo::Logger::GetInstance().logError(message, __FILE__, __LINE__)
#define LOG_CRITICAL_FULL(message)	Limbo::Logger::GetInstance().logCritical(message, __FILE__, __LINE__)

#define LOG_TRACE(message)			Limbo::Logger::GetInstance().logTrace(message)
#define LOG_DEBUG(message)			Limbo::Logger::GetInstance().logDebug(message)
#define LOG_INFO(message)			Limbo::Logger::GetInstance().logInfo(message)
#define LOG_WARNING(message)		Limbo::Logger::GetInstance().logWarning(message)
#define LOG_ERROR(message)			Limbo::Logger::GetInstance().logError(message)
#define LOG_CRITICAL(message)		Limbo::Logger::GetInstance().logCritical(message)

#define LOG_TRACE_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logTrace(message); }
#define LOG_DEBUG_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logDebug(message); }
#define LOG_INFO_IF(condition, message)			if (condition) { Limbo::Logger::GetInstance().logInfo(message); }
#define LOG_WARNING_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logWarning(message); }
#define LOG_ERROR_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logError(message); }
#define LOG_CRITICAL_IF(condition, message)		if (condition) { Limbo::Logger::GetInstance().logCritical(message); }

#define BIT(x) (1 << x)