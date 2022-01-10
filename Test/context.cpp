#include "context.h"
#include <GL/glew.h>
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <codecvt>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#ifdef _WIN32
#include <GLFW/glfw3native.h>
#include "windows.h"
#endif

std::string ws2s(const std::wstring& wstr) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

GLFWContext::Context::Context(const WindowSettings& windowSettings, const DeviceSettings& deviceSettings) {
    /* Window creation */
    device = std::make_shared<Device>(deviceSettings);
    window = std::make_shared<Window>(device, windowSettings);
#ifdef WIN32
    std::function<void(EDeviceError, std::wstring message)> onError = [](EDeviceError, std::wstring message) {};
#else
    std::function<void(EDeviceError, std::string message)> onError = [](EDeviceError, std::string message) {};
#endif
    errorListenerID = Device::ErrorEvent.AddListener(onError);

    window->MakeCurrentContext(true);
    device->SetVsync(true);
    driver = std::make_shared<Driver>(DriverSettings());
    glEnable(GL_MULTISAMPLE);
    window->MakeCurrentContext(false);
}

GLFWContext::Context::~Context() { Device::ErrorEvent.RemoveListener(errorListenerID); }
