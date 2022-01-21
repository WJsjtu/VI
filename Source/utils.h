#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <functional>
#include "fmt/core.h"
#include "fmt/printf.h"
#include "VI.h"

namespace Utils {

void SetGlobalLogger(VI::Logger* logger);

VI::Logger* GetGlobalLogger();

#ifdef _WIN32
std::string WideCharToMultiByteString(const wchar_t* wszBuffer);
#endif

template <typename... T>
void LoggerPrintf(VI::LogLevel level, const char* fmtStr, T&&... args) {
    std::string content = fmt::sprintf(fmtStr, args...);
    GetGlobalLogger()->onMessage(static_cast<VI::LogLevel>(level), content.c_str());
}
}  // namespace Utils

#define CV_LOG_INFO(flag, stream)                                  \
    {                                                              \
        std::ostringstream ss;                                     \
        ss << stream << std::endl;                                 \
        Utils::LoggerPrintf(VI::LogLevel::Info, ss.str().c_str()); \
    }
#define CV_LOG_DEBUG(flag, stream)                                 \
    {                                                              \
        std::ostringstream ss;                                     \
        ss << stream << std::endl;                                 \
        Utils::LoggerPrintf(VI::LogLevel::Info, ss.str().c_str()); \
    }
#define CV_LOG_ERROR(flag, stream)                                  \
    {                                                               \
        std::ostringstream ss;                                      \
        ss << stream << std::endl;                                  \
        Utils::LoggerPrintf(VI::LogLevel::Error, ss.str().c_str()); \
    }
#define CV_LOG_WARN(flag, stream)                                     \
    {                                                                 \
        std::ostringstream ss;                                        \
        ss << stream << std::endl;                                    \
        Utils::LoggerPrintf(VI::LogLevel::Warning, ss.str().c_str()); \
    }
