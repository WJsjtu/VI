#pragma once
#include <stdint.h>
#include <iostream>
#include <string>
#include <unordered_map>

namespace GLFWContext {

struct DriverSettings {
#ifdef _DEBUG
    bool debugMode = true;
#else
    bool debugMode = false;
#endif  // DEBUG
};

/**
 * The Driver represents the OpenGL context
 */
class Driver {
public:
    /**
     * Creates the Driver (OpenGL context)
     * @param p_driverSettings
     */
    Driver(const DriverSettings& p_driverSettings);

    Driver(const Driver&) = delete;

    Driver& operator=(Driver&) = delete;

    /**
     * Destroy the driver
     */
    ~Driver() = default;

    /**
     * Returns true if the OpenGL context is active
     */
    bool IsActive() const;

private:
    void InitGlew();
#ifdef WIN32
    static void __stdcall GLDebugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const char* message, const void* userParam);
#else
    static void GLDebugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const char* message, const void* userParam);
#endif

private:
    bool m_isActive;
};

}  // namespace GLFWContext