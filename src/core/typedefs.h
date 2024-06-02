#pragma once
#include "logger.h"

#define LOG_TRACE(message) Limbo::Logger::GetInstance().logTrace(message, __FILE__, __LINE__)
#define LOG_DEBUG(message) Limbo::Logger::GetInstance().logDebug(message, __FILE__, __LINE__)
#define LOG_INFO(message) Limbo::Logger::GetInstance().logInfo(message, __FILE__, __LINE__)
#define LOG_WARNING(message) Limbo::Logger::GetInstance().logWarning(message, __FILE__, __LINE__)
#define LOG_ERROR(message) Limbo::Logger::GetInstance().logError(message, __FILE__, __LINE__)
#define LOG_CRITICAL(message) Limbo::Logger::GetInstance().logCritical(message, __FILE__, __LINE__)