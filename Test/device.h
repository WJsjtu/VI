#pragma once
#include <stdint.h>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

struct GLFWcursor;

namespace GLFWContext {

class Context;

namespace Cursor {
/**
 * Some cursor modes.
 * They defines if the mouse pointer should be visible, locked or normal
 */
enum class ECursorMode { NORMAL = 0x00034001, DISABLED = 0x00034003, HIDDEN = 0x00034002 };

/**
 * Some cursor shapes.
 * They specify how the mouse pointer should look
 */
enum class ECursorShape { ARROW = 0x00036001, IBEAM = 0x00036002, CROSSHAIR = 0x00036003, HAND = 0x00036004, HRESIZE = 0x00036005, VRESIZE = 0x00036006 };
}  // namespace Cursor

namespace Eventing {
/**
 * The ID of a listener (Registered callback).
 * This value is needed to remove a listener from an event
 */
using ListenerID = uint64_t;

/**
 * A simple event that contains a set of function callbacks. These functions will be called on invoke
 */
template <class... ArgTypes>
class Event {
public:
    /**
     * Simple shortcut for a generic function without return value
     */
    using Callback = std::function<void(ArgTypes...)>;

    /**
     * Add a function callback to this event
     * Also return the ID of the new listener (You should store the returned ID if you want to remove the listener
     * later)
     * @param p_call
     */
    ListenerID AddListener(Callback p_callback);

    /**
     * Add a function callback to this event
     * Also return the ID of the new listener (You should store the returned ID if you want to remove the listener
     * later)
     * @param p_call
     */
    ListenerID operator+=(Callback p_callback);

    /**
     * Remove a function callback to this event using a Listener (Created when calling AddListener)
     * @param p_listener
     */
    bool RemoveListener(ListenerID p_listenerID);

    /**
     * Remove a function callback to this event using a Listener (Created when calling AddListener)
     * @param p_listener
     */
    bool operator-=(ListenerID p_listenerID);

    /**
     * Remove every listeners to this event
     */
    void RemoveAllListeners();

    /**
     * Return the number of callback registered
     */
    uint64_t GetListenerCount();

    /**
     * Call every callbacks attached to this event
     * @param p_args (Variadic)
     */
    void Invoke(ArgTypes... p_args);

private:
    std::unordered_map<ListenerID, Callback> m_callbacks;
    ListenerID m_availableListenerID = 0;
};

template <class... ArgTypes>
ListenerID Event<ArgTypes...>::AddListener(Callback p_callback) {
    ListenerID listenerID = m_availableListenerID++;
    m_callbacks.emplace(listenerID, p_callback);
    return listenerID;
}

template <class... ArgTypes>
ListenerID Event<ArgTypes...>::operator+=(Callback p_callback) {
    return AddListener(p_callback);
}

template <class... ArgTypes>
bool Event<ArgTypes...>::RemoveListener(ListenerID p_listenerID) {
    return m_callbacks.erase(p_listenerID) != 0;
}

template <class... ArgTypes>
bool Event<ArgTypes...>::operator-=(ListenerID p_listenerID) {
    return RemoveListener(p_listenerID);
}

template <class... ArgTypes>
void Event<ArgTypes...>::RemoveAllListeners() {
    m_callbacks.clear();
}

template <class... ArgTypes>
uint64_t Event<ArgTypes...>::GetListenerCount() {
    return m_callbacks.size();
}

template <class... ArgTypes>
void Event<ArgTypes...>::Invoke(ArgTypes... p_args) {
    for (auto const& pair : m_callbacks) pair.second(p_args...);
}

}  // namespace Eventing

/**
 * Contains device settings
 */
struct DeviceSettings {
    DeviceSettings() = default;

    DeviceSettings(const DeviceSettings&) = default;

    DeviceSettings& operator=(DeviceSettings&) = default;

    /**
     * specifies whether to create a debug OpenGL context, which may have additional error and
     * performance issue reporting functionality. If OpenGL ES is requested, this hint is ignored
     */
#ifdef _DEBUG
    bool debugProfile = true;
#else
    bool debugProfile = false;
#endif

    /**
     * Specifies whether the OpenGL context should be forward-compatible, i.e. one where all functionality
     * deprecated in the requested version of OpenGL is removed. This must only be used if the requested OpenGL
     * version is 3.0 or above. If OpenGL ES is requested, this hint is ignored.
     */
    bool forwardCompatibility = false;

    /**
     * Specify the client API major version that the created context must be compatible with. The exact
     * behavior of these hints depend on the requested client API
     */
    uint8_t contextMajorVersion = 3;

    /**
     * Specify the client API minor version that the created context must be compatible with. The exact
     * behavior of these hints depend on the requested client API
     */
    uint8_t contextMinorVersion = 2;

    /**
     * Defines the amount of samples to use (Requiered for multi-sampling)
     */
    uint8_t samples = 4;
};

/**
 * Some errors that the driver can return
 */
enum class EDeviceError {
    NOT_INITIALIZED = 0x00010001,
    NO_CURRENT_CONTEXT = 0x00010002,
    INVALID_ENUM = 0x00010003,
    INVALID_VALUE = 0x00010004,
    OUT_OF_MEMORY = 0x00010005,
    API_UNAVAILABLE = 0x00010006,
    VERSION_UNAVAILABLE = 0x00010007,
    PLATFORM_ERROR = 0x00010008,
    FORMAT_UNAVAILABLE = 0x00010009,
    NO_WINDOW_CONTEXT = 0x0001000A
};

/**
 * The Device represents the windowing context. It is necessary to create a device
 * to create a window
 */
class Device {
public:
    /**
     * The constructor of the device will take care about GLFW initialization
     */
    Device(const DeviceSettings& p_deviceSettings);

    Device(const Device&) = delete;

    Device& operator=(Device&) = delete;

    /**
     * The destructor of the device will take care about GLFW destruction
     */
    ~Device();

    /**
     * Return the size, in pixels, of the primary monity
     */
    std::pair<int16_t, int16_t> GetMonitorSize() const;

    /**
     * Return an instance of GLFWcursor corresponding to the given shape
     * @param p_cursorShape
     */
    GLFWcursor* GetCursorInstance(Cursor::ECursorShape p_cursorShape) const;

    /**
     * Return true if the vsync is currently enabled
     */
    bool HasVsync() const;

    /**
     * Enable or disable the vsync
     * @note You must call this method after creating and defining a window as the current context
     * @param p_value (True to enable vsync)
     */
    void SetVsync(bool p_value);

    /**
     * Enable the inputs and events managments with created windows
     * @note Should be called every frames
     */
    void PollEvents() const;

    /**
     * Returns the elapsed time since the device startup
     */
    float GetElapsedTime() const;

private:
    void BindErrorCallback();
    void CreateCursors();
    void DestroyCursors();

private:
    /**
     * Bind a listener to this event to receive device errors
     */
#ifdef WIN32
    static Eventing::Event<EDeviceError, std::wstring> ErrorEvent;
#else
    static Eventing::Event<EDeviceError, std::string> ErrorEvent;
#endif

    bool m_vsync = true;
    bool m_isAlive = false;
    std::unordered_map<Cursor::ECursorShape, GLFWcursor*> m_cursors;

    friend class Context;
};

}  // namespace GLFWContext