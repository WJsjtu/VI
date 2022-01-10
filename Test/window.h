#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>
#include "device.h"

struct GLFWwindow;

namespace GLFWContext {
/**
 * Contains window settings
 */
struct WindowSettings {
    WindowSettings() = default;

    WindowSettings(const WindowSettings&) = default;

    WindowSettings& operator=(WindowSettings&) = default;

    /**
     * A simple constant used to ignore a value setting (Let the program decide for you)
     * @note You can you WindowSettings::DontCare only where it is indicated
     */
    static const int32_t DontCare = -1;

    /**
     * Title of the window (Displayed in the title bar)
     */
    std::string title;

    /**
     * Width in pixels of the window
     */
    uint16_t width;

    /**
     * Height in pixels of the window
     */
    uint16_t height;

    /**
     * Minimum width of the window.
     * Use WindowSettings::DontCare to disable limit
     */
    int16_t minimumWidth = DontCare;

    /**
     * Minimum height of the window.
     * Use WindowSettings::DontCare to disable limit
     */
    int16_t minimumHeight = DontCare;

    /**
     * Maximum width of the window.
     * Use WindowSettings::DontCare to disable limit
     */
    int16_t maximumWidth = DontCare;

    /**
     * Maximum height of the window.
     * Use WindowSettings::DontCare to disable limit
     */
    int16_t maximumHeight = DontCare;

    /**
     * Specifies if the window is by default in fullscreen or windowed mode
     */
    bool fullscreen = false;

    /**
     * Specifies whether the windowed mode window will have window decorations such as a border, a close widget, etc.
     * An undecorated window may still allow the user to generate close events on some platforms. This hint is ignored
     * for full screen windows.
     */
    bool decorated = true;

    /**
     * specifies whether the windowed mode window will be resizable by the user. The window will still be resizable
     * using the "SetSize(uint16_t, uint16_t)" method of the "Window" class. This hint is ignored for full screen
     * windows
     */
    bool resizable = true;

    /**
     * Specifies whether the windowed mode window will be given input focus when created. This hint is ignored for
     * full screen and initially hidden windows.
     */
    bool focused = true;

    /**
     * Specifies whether the windowed mode window will be maximized when created. This hint is ignored for full screen
     * windows.
     */
    bool maximized = false;

    /**
     * Specifies whether the windowed mode window will be floating above other regular windows, also called topmost or
     * always-on-top. This is intended primarily for debugging purposes and cannot be used to implement proper full
     * screen windows. This hint is ignored for full screen windows.
     */
    bool floating = false;

    /**
     * Specifies whether the windowed mode window will be initially visible. This hint is ignored for full screen
     * windows.
     */
    bool visible = true;

    /**
     * Specifies whether the full screen window will automatically iconify and restore
     * the previous video mode on input focus loss. This hint is ignored for windowed mode windows
     */
    bool autoIconify = true;

    /**
     * Specifies the desired refresh rate for full screen windows. If set to WindowSettings::DontCare, the highest
     * available refresh rate will be used. This hint is ignored for windowed mode windows.
     */
    int32_t refreshRate = WindowSettings::DontCare;

    /**
     * Specifies the default cursor mode of the window
     */
    Cursor::ECursorMode cursorMode = Cursor::ECursorMode::NORMAL;

    /**
     * Specifies the default cursor shape of the window
     */
    Cursor::ECursorShape cursorShape = Cursor::ECursorShape::ARROW;

    /**
     * Defines the number of samples to use (For anti-aliasing)
     */
    uint32_t samples = 4;
};

/**
 * A simple OS-based window.
 * It needs a Device (GLFW) to work
 */
class Window {
public:
    /* Inputs relatives */
    Eventing::Event<int> KeyPressedEvent;
    Eventing::Event<int> KeyReleasedEvent;
    Eventing::Event<int> MouseButtonPressedEvent;
    Eventing::Event<int> MouseButtonReleasedEvent;

    /* Window events */
    Eventing::Event<uint16_t, uint16_t> ResizeEvent;
    Eventing::Event<uint16_t, uint16_t> FramebufferResizeEvent;
    Eventing::Event<int16_t, int16_t> MoveEvent;
    Eventing::Event<int16_t, int16_t> CursorMoveEvent;
    Eventing::Event<> MinimizeEvent;
    Eventing::Event<> MaximizeEvent;
    Eventing::Event<> GainFocusEvent;
    Eventing::Event<> LostFocusEvent;
    Eventing::Event<> CloseEvent;

    /**
     * Create the window
     * @param p_device
     * @param p_windowSettings
     */
    Window(const std::shared_ptr<Device>& p_device, const WindowSettings& p_windowSettings);

    Window(const Window&) = delete;

    Window& operator=(Window&) = delete;

    /**
     * Destructor of the window, responsible of the GLFW window memory free
     */
    ~Window();

    /**
     * Set Icon from memory
     * @param p_data
     * @param p_width
     * @param p_height
     */
    void SetIconFromMemory(uint8_t* p_data, uint32_t p_width, uint32_t p_height);

    /**
     * Find an instance of window with a given GLFWwindow
     * @param p_glfwWindow
     */
    static Window* FindInstance(GLFWwindow* p_glfwWindow);

    /**
     * Resize the window
     * @param p_width
     * @param p_height
     */
    void SetSize(uint16_t p_width, uint16_t p_height);

    /**
     * Defines a minimum size for the window
     * @param p_minimumWidth
     * @param p_minimumHeight
     * @note -1 (WindowSettings::DontCare) value means no limitation
     */
    void SetMinimumSize(int16_t p_minimumWidth, int16_t p_minimumHeight);

    /**
     * Defines a maximum size for the window
     * @param p_maximumWidth
     * @param p_maximumHeight
     * @note -1 (WindowSettings::DontCare) value means no limitation
     */
    void SetMaximumSize(int16_t p_maximumWidth, int16_t p_maximumHeight);

    /**
     * Define a position for the window
     * @param p_x
     * @param p_y
     */
    void SetPosition(int16_t p_x, int16_t p_y);

    /**
     * Minimize the window
     */
    void Minimize() const;

    /**
     * Maximize the window
     */
    void Maximize() const;

    /**
     * Restore the window
     */
    void Restore() const;

    /**
     * Hides the specified window if it was previously visible
     */
    void Hide() const;

    /**
     * Show the specified window if it was previously hidden
     */
    void Show() const;

    /**
     * Focus the window
     */
    void Focus() const;

    /**
     * Set the should close flag of the window to true
     * @param p_value
     */
    void SetShouldClose(bool p_value) const;

    /**
     * Return true if the window should close
     */
    bool ShouldClose() const;

    /**
     * Set the window in fullscreen or windowed mode
     * @param p_value (True for fullscreen mode, false for windowed)
     */
    void SetFullscreen(bool p_value);

    /**
     * Switch the window to fullscreen or windowed mode depending
     * on the current state
     */
    void ToggleFullscreen();

    /**
     * Return true if the window is fullscreen
     */
    bool IsFullscreen() const;

    /**
     * Return true if the window is hidden
     */
    bool IsHidden() const;

    /**
     * Return true if the window is visible
     */
    bool IsVisible() const;

    /**
     * Return true if the windows is maximized
     */
    bool IsMaximized() const;

    /**
     * Return true if the windows is minimized
     */
    bool IsMinimized() const;

    /**
     * Return true if the windows is focused
     */
    bool IsFocused() const;

    /**
     * Return true if the windows is resizable
     */
    bool IsResizable() const;

    /**
     * Return true if the windows is decorated
     */
    bool IsDecorated() const;

    /**
     * Define the window as the current context
     */
    void MakeCurrentContext(bool active);

    /**
     * Handle the buffer swapping with the current window
     */
    void SwapBuffers() const;

    /**
     * Define a mode for the mouse cursor
     * @param p_cursorMode
     */
    void SetCursorMode(Cursor::ECursorMode p_cursorMode);

    /**
     * Define a shape to apply to the current cursor
     * @param p_cursorShape
     */
    void SetCursorShape(Cursor::ECursorShape p_cursorShape);

    /**
     * Move the cursor to the given position
     */
    void SetCursorPosition(int16_t p_x, int16_t p_y);

    /**
     * Define a title for the window
     * @param p_title
     */
    void SetTitle(const std::string& p_title);

    /**
     * Defines a refresh rate (Use WindowSettings::DontCare to use the highest available refresh rate)
     * @param p_refreshRate
     * @note You need to switch to fullscreen mode to apply this effect (Or leave fullscreen and re-apply)
     */
    void SetRefreshRate(int32_t p_refreshRate);

    /**
     * Return the title of the window
     */
    std::string GetTitle() const;

    /**
     * Return the current size of the window
     */
    std::pair<uint16_t, uint16_t> GetSize() const;

    /**
     * Return the current minimum size of the window
     * @note -1 (WindowSettings::DontCare) values means no limitation
     */
    std::pair<int16_t, int16_t> GetMinimumSize() const;

    /**
     * Return the current maximum size of the window
     * @note -1 (WindowSettings::DontCare) values means no limitation
     */
    std::pair<int16_t, int16_t> GetMaximumSize() const;

    /**
     * Return the current position of the window
     */
    std::pair<int16_t, int16_t> GetPosition() const;

    /**
     * Return the framebuffer size (Viewport size)
     */
    std::pair<uint16_t, uint16_t> GetFramebufferSize() const;

    /**
     * Return the current cursor mode
     */
    Cursor::ECursorMode GetCursorMode() const;

    /**
     * Return the current cursor shape
     */
    Cursor::ECursorShape GetCursorShape() const;

    /**
     * Return the current refresh rate (Only applied to the fullscreen mode).
     * If the value is -1 (WindowSettings::DontCare) the highest refresh rate will be used
     */
    int32_t GetRefreshRate() const;

    /**
     * Return GLFW window
     */
    GLFWwindow* GetGlfwWindow() const;

private:
    void CreateGlfwWindow(const WindowSettings& p_windowSettings);

    /* Callbacks binding */
    void BindKeyCallback() const;
    void BindMouseCallback() const;
    void BindResizeCallback() const;
    void BindFramebufferResizeCallback() const;
    void BindCursorMoveCallback() const;
    void BindMoveCallback() const;
    void BindIconifyCallback() const;
    void BindFocusCallback() const;
    void BindCloseCallback() const;

    /* Event listeners */
    void OnResize(uint16_t p_width, uint16_t p_height);
    void OnMove(int16_t p_x, int16_t p_y);

    /* Internal helpers */
    void UpdateSizeLimit() const;

private:
    /* This map is used by callbacks to find a "Window" instance out of a "GLFWwindow" instance*/
    static std::unordered_map<GLFWwindow*, Window*> __WINDOWS_MAP;

    std::shared_ptr<Device> m_device;
    GLFWwindow* m_glfwWindow;

    /* Window settings */
    std::string m_title;
    std::pair<uint16_t, uint16_t> m_size;
    std::pair<int16_t, int16_t> m_minimumSize;
    std::pair<int16_t, int16_t> m_maximumSize;
    std::pair<int16_t, int16_t> m_position;
    bool m_fullscreen;
    int32_t m_refreshRate;
    Cursor::ECursorMode m_cursorMode;
    Cursor::ECursorShape m_cursorShape;
};

}  // namespace GLFWContext