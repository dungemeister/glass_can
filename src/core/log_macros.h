#pragma once
#include "logger.h"

#define LOG_INFO(msg)   logging::Logger::getInstance().log(logging::LoggingLevel::INFO,    msg)
#define LOG_DEBUG(msg)  logging::Logger::getInstance().log(logging::LoggingLevel::DEBUG,   msg)
#define LOG_WARN(msg)   logging::Logger::getInstance().log(logging::LoggingLevel::WARNING, msg)
#define LOG_ERROR(msg)  logging::Logger::getInstance().log(logging::LoggingLevel::ERROR,   msg)
