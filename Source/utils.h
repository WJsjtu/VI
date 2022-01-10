#pragma once
#include "fmt/core.h"
#include "fmt/printf.h"
#include <iostream>
#include <string>

namespace Utils {
	enum class LogLevel {
		Debug = 1,
		Info = 2,
		Warning = 3,
		Error = 4
	};

	typedef void(*LoggerFunction)(LogLevel, const char*);

	void SetGlobalLogger(LoggerFunction fn);

	LoggerFunction GetGlobalLogger();

#ifdef _WIN32
	std::string WideCharToMultiByteString(const wchar_t* wszBuffer);
#endif

	template <typename... T>
	void LoggerPrintf(LogLevel level, const char* fmtStr, T&&... args) {
		std::string content = fmt::sprintf(fmtStr, args...);
		GetGlobalLogger()(level, content.c_str());
	}
}