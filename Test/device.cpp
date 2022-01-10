#include "device.h"
#include <GL/glew.h>
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#include <codecvt>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>

#if defined(WIN32)
#include "windows.h"
#endif

#ifdef WIN32
GLFWContext::Eventing::Event<GLFWContext::EDeviceError, std::wstring> GLFWContext::Device::ErrorEvent;
#else
GLFWContext::Eventing::Event<GLFWContext::EDeviceError, std::string> GLFWContext::Device::ErrorEvent;
#endif

GLFWContext::Device::Device(const DeviceSettings& p_deviceSettings) {
    BindErrorCallback();

    int initializationCode = glfwInit();

    if (initializationCode == GLFW_FALSE) {
        throw std::runtime_error("Failed to Init GLFW");
        glfwTerminate();
    } else {
        CreateCursors();
#ifdef WIN32
        if (p_deviceSettings.debugProfile) glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, p_deviceSettings.contextMajorVersion);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, p_deviceSettings.contextMinorVersion);

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, p_deviceSettings.samples);

        m_isAlive = true;
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, p_deviceSettings.samples);
#endif
    }
}

GLFWContext::Device::~Device() {
    if (m_isAlive) {
        DestroyCursors();
        glfwTerminate();
    }
}

std::pair<int16_t, int16_t> GLFWContext::Device::GetMonitorSize() const {
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    return std::pair<int16_t, int16_t>(static_cast<int16_t>(mode->width), static_cast<int16_t>(mode->height));
}

GLFWcursor* GLFWContext::Device::GetCursorInstance(Cursor::ECursorShape p_cursorShape) const { return m_cursors.at(p_cursorShape); }

bool GLFWContext::Device::HasVsync() const { return m_vsync; }

void GLFWContext::Device::SetVsync(bool p_value) {
    glfwSwapInterval(p_value ? 1 : 0);
    m_vsync = p_value;
}

void GLFWContext::Device::PollEvents() const { glfwPollEvents(); }

float GLFWContext::Device::GetElapsedTime() const { return static_cast<float>(glfwGetTime()); }

void GLFWContext::Device::BindErrorCallback() {
    auto errorCallback = [](int p_code, const char* p_description) {
#ifdef WIN32
        WCHAR buffer[1024] = L"";
        MultiByteToWideChar(CP_UTF8, 0, p_description, -1, buffer, sizeof(buffer) / 2);
        std::wstring message(buffer);
        ErrorEvent.Invoke(static_cast<EDeviceError>(p_code), message);
#else
        ErrorEvent.Invoke(static_cast<EDeviceError>(p_code), p_description);
#endif
    };

    glfwSetErrorCallback(errorCallback);
}

void GLFWContext::Device::CreateCursors() {
    m_cursors[Cursor::ECursorShape::ARROW] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::ARROW));
    m_cursors[Cursor::ECursorShape::IBEAM] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::IBEAM));
    m_cursors[Cursor::ECursorShape::CROSSHAIR] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::CROSSHAIR));
    m_cursors[Cursor::ECursorShape::HAND] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::HAND));
    m_cursors[Cursor::ECursorShape::HRESIZE] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::HRESIZE));
    m_cursors[Cursor::ECursorShape::VRESIZE] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::VRESIZE));
}

void GLFWContext::Device::DestroyCursors() {
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::ARROW]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::IBEAM]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::CROSSHAIR]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::HAND]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::HRESIZE]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::VRESIZE]);
}
