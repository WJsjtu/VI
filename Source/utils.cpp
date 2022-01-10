#include "utils.h"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace Utils {
	void DefaultLoggerFunction(LogLevel level, const char* msg) {
		switch (level)
		{
		case LogLevel::Debug:
			std::cout << msg;
			break;
		case LogLevel::Info:
			std::cout << msg;
			break;
		case LogLevel::Warning:
			std::cout << msg;
			break;
		case LogLevel::Error:
			std::cerr << msg;
			break;
		default:
			break;
		}
	}

	LoggerFunction GlobalLoggerFunction = DefaultLoggerFunction;

	void SetGlobalLogger(LoggerFunction fn) {
		GlobalLoggerFunction = fn;
	}

	LoggerFunction GetGlobalLogger() {
		return GlobalLoggerFunction;
	}

#ifdef _WIN32
	std::string WideCharToMultiByteString(const wchar_t* wszBuffer) {
		std::string result;
		int nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
		CHAR* szBuffer = new CHAR[nLen + 1];
		nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
		szBuffer[nLen] = 0;
		result = std::string(szBuffer);
		delete[] szBuffer;
		return result;
	}
#endif
}