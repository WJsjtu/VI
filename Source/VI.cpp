#include "VI.h"
#ifdef _WIN32
#include "videoInput.h"
#elif __APPLE__
#include "avfoundation.h"
#endif
#include "precomp.hpp"
#include "utils.h"
#include "videoio.hpp"
#include <unordered_map>
#include <mutex>

namespace VI {
String::String() {
    m_data = new char[1];
    m_data[0] = '\0';
    m_size = 0;
}
String::String(const char* str, size_t size) {
    if (size == 0) {
        size = strlen(str);
    }
    m_size = size;
    m_data = new char[m_size + 1];
    memcpy(m_data, str, m_size);
    m_data[m_size] = '\0';
}
String::String(const String& str) {
    m_size = str.m_size;
    m_data = new char[m_size + 1];
    memcpy(m_data, str.m_data, str.m_size);
    m_data[m_size] = '\0';
}
String::~String() { delete[] m_data; }
String& String::operator=(const String& str) {
    if (this == &str) return *this;
    delete[] m_data;
    m_size = (str.m_size);
    m_data = new char[m_size + 1];
    memcpy(m_data, str.m_data, str.m_size);
    m_data[m_size] = '\0';
    return *this;
}
bool String::operator==(const char* str) { return m_size == strlen(str) ? strncmp(m_data, str, m_size) == 0 : false; }
bool String::operator==(const String& str) { return m_size == str.m_size ? strncmp(m_data, str.m_data, m_size) == 0 : false; }
const char* String::data() const { return m_data; }
const size_t String::size() const { return m_size; }

#ifdef _WIN32

std::unordered_map<int, uint32_t> GlobalDeviceUsage;
std::mutex GlobalDeviceUsageMutex;

std::shared_ptr<videoInput> GlobalVideoInput = nullptr;

videoInput& GetGlobalVideoInput() {
    if (!GlobalVideoInput) {
        GlobalVideoInput = std::make_shared<videoInput>();
    }
    return *GlobalVideoInput;
}

struct CameraInfo {
    int deviceID;
    bool bSuccess;
};

void Camera::setVerbose(bool verbose) { videoInput::setVerbose(verbose); }
void Camera::setComMultiThreaded(bool bMulti) { videoInput::setComMultiThreaded(bMulti); }

Camera::Camera(int deviceID) {
    int numDevices = GetGlobalVideoInput().listDevices();
    GetGlobalVideoInput().setUseCallback(true);
    bool success = false;
    if (deviceID >= 0 && deviceID < numDevices) {
        std::lock_guard<std::mutex> lk(GlobalDeviceUsageMutex);
        if (GlobalDeviceUsage.find(deviceID) == GlobalDeviceUsage.end()) {
            success = GetGlobalVideoInput().setupDevice(deviceID);
            GlobalDeviceUsage.emplace(deviceID, 0);
        } else {
            GlobalDeviceUsage[deviceID]++;
            success = GetGlobalVideoInput().isDeviceSetup(deviceID);
        }
    } else {
        deviceID = -1;
        success = false;
    }
    auto info = new CameraInfo();
    info->deviceID = deviceID;
    info->bSuccess = success;
    m_handle = info;
}

Camera::Camera(int deviceID, int width, int height, int fps) {
    int numDevices = GetGlobalVideoInput().listDevices();
    GetGlobalVideoInput().setUseCallback(true);
    bool success = false;
    if (deviceID >= 0 && deviceID < numDevices) {
        std::lock_guard<std::mutex> lk(GlobalDeviceUsageMutex);
        if (GlobalDeviceUsage.find(deviceID) == GlobalDeviceUsage.end()) {
            success = GetGlobalVideoInput().setupDevice(deviceID, width, height);
            GlobalDeviceUsage.emplace(deviceID, 1);
        } else {
            GlobalDeviceUsage[deviceID]++;
            success = GetGlobalVideoInput().isDeviceSetup(deviceID);
        }
    } else {
        deviceID = -1;
        success = false;
    }
    auto info = new CameraInfo();
    info->deviceID = deviceID;
    info->bSuccess = success;
    if (success && fps) {
        GetGlobalVideoInput().setIdealFramerate(deviceID, fps);
    }
    m_handle = info;
}

Camera::~Camera() {
    if (m_handle) {
        int deviceID = ((CameraInfo*)(m_handle))->deviceID;
        if (deviceID >= 0) {
            std::lock_guard<std::mutex> lk(GlobalDeviceUsageMutex);
            auto usage = GlobalDeviceUsage.find(deviceID);
            if (usage != GlobalDeviceUsage.end()) {
                usage->second--;
                if (usage->second == 0) {
                    GlobalDeviceUsage.erase(usage);
                    GetGlobalVideoInput().stopDevice(deviceID);
                }
            }
        }
        delete ((CameraInfo*)(m_handle));
        m_handle = NULL;
    }
}

// Functions in rough order they should be used.
LinkedList<String> Camera::getDeviceList() {
    auto list = videoInput::getDeviceList();
    LinkedList<String> result;
    for (auto& item : list) {
        String name(item.c_str(), item.size());
        result.add(name);
    }
    return result;
}

// Tells you when a new frame has arrived - you should call this if you have
// specified setAutoReconnectOnFreeze to true
bool Camera::isFrameNew() {
    if (!((CameraInfo*)(m_handle))->bSuccess) {
        return false;
    }
    int deviceID = ((CameraInfo*)(m_handle))->deviceID;
    if (deviceID >= 0) {
        return GetGlobalVideoInput().isFrameNew(deviceID);
    } else {
        return false;
    }
}

bool Camera::isDeviceSetup() {
    if (!((CameraInfo*)(m_handle))->bSuccess) {
        return false;
    }
    int deviceID = ((CameraInfo*)(m_handle))->deviceID;
    if (deviceID >= 0) {
        return GetGlobalVideoInput().isDeviceSetup(deviceID);
    } else {
        return false;
    }
}

// Or pass in a buffer for getPixels to fill returns true if successful.
bool Camera::getPixels(unsigned char* pixels, bool rgb, bool flipX, bool flipY) {
    if (!((CameraInfo*)(m_handle))->bSuccess) {
        return false;
    }
    int deviceID = ((CameraInfo*)(m_handle))->deviceID;
    if (deviceID >= 0) {
        return GetGlobalVideoInput().getPixels(deviceID, pixels, rgb, flipX, !flipY);
    } else {
        return false;
    }
}

// Launches a pop up settings window
// For some reason in GLUT you have to call it twice each time.
void Camera::showSettingsWindow() {
    if (!((CameraInfo*)(m_handle))->bSuccess) {
        return;
    }
    int deviceID = ((CameraInfo*)(m_handle))->deviceID;
    if (deviceID >= 0) {
        return GetGlobalVideoInput().showSettingsWindow(deviceID);
    } else {
        return;
    }
}

// get width, height and number of pixels
int Camera::getWidth() {
    if (!((CameraInfo*)(m_handle))->bSuccess) {
        return 0;
    }
    int deviceID = ((CameraInfo*)(m_handle))->deviceID;
    if (deviceID >= 0) {
        return GetGlobalVideoInput().getWidth(deviceID);
    } else {
        return 0;
    }
}

int Camera::getHeight() {
    if (!((CameraInfo*)(m_handle))->bSuccess) {
        return 0;
    }
    int deviceID = ((CameraInfo*)(m_handle))->deviceID;
    if (deviceID >= 0) {
        return GetGlobalVideoInput().getHeight(deviceID);
    } else {
        return 0;
    }
}

#elif __APPLE__
LinkedList<String> Camera::getDeviceList() {
    auto list = videoInput::getDeviceList();
    LinkedList<String> result;
    for (auto& item : list) {
        String name(item.c_str(), item.size());
        result.add(name);
    }
    return result;
}

Camera::Camera(int deviceID) { m_handle = new videoInput(deviceID); }

Camera::Camera(int deviceID, int width, int height, int fps) {
    m_handle = new videoInput(deviceID);
    ((videoInput*)(m_handle))->setProperty(CV_CAP_PROP_FRAME_WIDTH, width);
    ((videoInput*)(m_handle))->setProperty(CV_CAP_PROP_FRAME_HEIGHT, height);
    if (fps) {
        ((videoInput*)(m_handle))->setProperty(CV_CAP_PROP_FPS, fps);
    }
}
Camera::~Camera() {
    if (m_handle) {
        delete (videoInput*)(m_handle);
        m_handle = NULL;
    }
}
bool Camera::getPixels(unsigned char* pixels, bool rgb, bool flipX, bool flipY) {
    ((videoInput*)(m_handle))->retrieveFrame(pixels, rgb, flipX, flipY);
    return true;
}
void Camera::showSettingsWindow() {}
int Camera::getWidth() { return ((videoInput*)(m_handle))->getProperty(CV_CAP_PROP_FRAME_WIDTH); }
int Camera::getHeight() { return ((videoInput*)(m_handle))->getProperty(CV_CAP_PROP_FRAME_HEIGHT); }
bool Camera::isDeviceSetup() { return ((videoInput*)(m_handle))->didStart(); }
bool Camera::isFrameNew() { return ((videoInput*)(m_handle))->grabFrame(); }
#endif

Video::Video(const String& file) {
    VideoCaptureParameters params;
    // params.add(CAP_PROP_HW_ACCELERATION, VIDEO_ACCELERATION_ANY);
    std::string path = file.data();
#if _WIN32
    for (auto& chr : path) {
        if (chr == '\\') {
            chr = '/';
        }
    }
#endif
    m_handle = cvCreateFileCapture_FFMPEG_proxy(path.c_str(), params);
    seekTime(0);
}

Video::~Video() {
    if (m_handle) {
        delete (IVideoCapture*)(m_handle);
        m_handle = nullptr;
    }
}

int64_t Video::getFramesCount() { return m_handle ? ((IVideoCapture*)(m_handle))->getProperty(CAP_PROP_FRAME_COUNT) : 0; }
double Video::getTotalTime() {
    auto fps = getFPS();
    return fps == 0 ? 0 : (double)(getFramesCount()) / fps;
}
double Video::getFPS() { return m_handle ? ((IVideoCapture*)(m_handle))->getProperty(CAP_PROP_FPS) : 0; }
int64_t Video::getBitRate() { return m_handle ? ((IVideoCapture*)(m_handle))->getProperty(CAP_PROP_BITRATE) : 0; }

int64_t Video::getCurrentFrame() { return m_handle ? ((IVideoCapture*)(m_handle))->getProperty(CAP_PROP_POS_FRAMES) : 0; }
double Video::getCurrentTime() { return m_handle ? ((IVideoCapture*)(m_handle))->getProperty(CAP_PROP_POS_MSEC) / 1000 : 0; }

int Video::getWidth() { return m_handle ? ((IVideoCapture*)(m_handle))->getProperty(CAP_PROP_FRAME_WIDTH) : 0; }
int Video::getHeight() { return m_handle ? ((IVideoCapture*)(m_handle))->getProperty(CAP_PROP_FRAME_HEIGHT) : 0; }

void Video::seekFrame(int64_t frame_number) {
    if (m_handle) {
        ((IVideoCapture*)(m_handle))->seek(frame_number);
    }
}
void Video::seekTime(double sec) {
    if (m_handle) {
        ((IVideoCapture*)(m_handle))->seek(sec);
    }
}

bool Video::retrieveFrame(double sec, unsigned char** data, bool rgb) {
    if (m_handle) {
        IVideoCaptureFrame frame;
        ((IVideoCapture*)(m_handle))->getProperty(CAP_PROP_POS_MSEC);
        ((IVideoCapture*)(m_handle))->seek(sec);
        if (((IVideoCapture*)(m_handle))->retrieveFrame(0, frame, rgb)) {
            memcpy(*data, frame.data, frame.width * frame.height * 3 * sizeof(char));
            return true;
        }
    }
    return false;
}

void SetGlobalLogger(Logger* logger) { Utils::SetGlobalLogger(logger); }
}  // namespace VI
