#include "utils.h"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace Utils {

class DefaultLogger : public VI::Logger {
public:
    virtual void onMessage(VI::LogLevel level, const char* msg) override {
        switch (level) {
            case VI::LogLevel::Debug: std::cout << msg; break;
            case VI::LogLevel::Info: std::cout << msg; break;
            case VI::LogLevel::Warning: std::cout << msg; break;
            case VI::LogLevel::Error: std::cerr << msg; break;
            default: break;
        }
    }
};

VI::Logger* GlobalLoggerFunction = NULL;

void SetGlobalLogger(VI::Logger* logger) { GlobalLoggerFunction = logger; }

VI::Logger* GetGlobalLogger() {
    static DefaultLogger GlobalDefaultLogger;
    return GlobalLoggerFunction ? GlobalLoggerFunction : &GlobalDefaultLogger;
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
}  // namespace Utils