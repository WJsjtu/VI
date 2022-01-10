#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include "device.h"
#include "driver.h"
#include "window.h"
#if defined(WIN32)
#include "windows.h"
#endif

namespace GLFWContext {
/**
 * The Context handle the engine features setup
 */
class Context {
public:
    /**
     * Constructor
     */
    Context(const WindowSettings& windowSettings, const DeviceSettings& deviceSettings);

    Context() = delete;

    Context(const Context&) = delete;

    Context& operator=(Context&) = delete;

    /**
     * Destructor
     */
    ~Context();

public:
    std::shared_ptr<Device> device;
    std::shared_ptr<Window> window;
    std::shared_ptr<Driver> driver;

private:
    Eventing::ListenerID errorListenerID;
};

}  // namespace GLFWContext