
#include "precomp.hpp"
#include <assert.h>
#include <string>

#include "cap_ffmpeg_impl.hpp"

enum VideoCaptureAPIs {
    CAP_ANY = 0,                  //!< Auto detect == 0
    CAP_VFW = 200,                //!< Video For Windows (obsolete, removed)
    CAP_V4L = 200,                //!< V4L/V4L2 capturing support
    CAP_V4L2 = CAP_V4L,           //!< Same as CAP_V4L
    CAP_FIREWIRE = 300,           //!< IEEE 1394 drivers
    CAP_FIREWARE = CAP_FIREWIRE,  //!< Same value as CAP_FIREWIRE
    CAP_IEEE1394 = CAP_FIREWIRE,  //!< Same value as CAP_FIREWIRE
    CAP_DC1394 = CAP_FIREWIRE,    //!< Same value as CAP_FIREWIRE
    CAP_CMU1394 = CAP_FIREWIRE,   //!< Same value as CAP_FIREWIRE
    CAP_QT = 500,                 //!< QuickTime (obsolete, removed)
    CAP_UNICAP = 600,             //!< Unicap drivers (obsolete, removed)
    CAP_DSHOW = 700,              //!< DirectShow (via videoInput)
    CAP_PVAPI = 800,              //!< PvAPI, Prosilica GigE SDK
    CAP_OPENNI = 900,             //!< OpenNI (for Kinect)
    CAP_OPENNI_ASUS = 910,        //!< OpenNI (for Asus Xtion)
    CAP_ANDROID = 1000,           //!< Android - not used
    CAP_XIAPI = 1100,             //!< XIMEA Camera API
    CAP_AVFOUNDATION = 1200,      //!< AVFoundation framework for iOS (OS X Lion will have the same API)
    CAP_GIGANETIX = 1300,         //!< Smartek Giganetix GigEVisionSDK
    CAP_MSMF = 1400,              //!< Microsoft Media Foundation (via videoInput)
    CAP_WINRT = 1410,             //!< Microsoft Windows Runtime using Media Foundation
    CAP_INTELPERC = 1500,         //!< RealSense (former Intel Perceptual Computing SDK)
    CAP_REALSENSE = 1500,         //!< Synonym for CAP_INTELPERC
    CAP_OPENNI2 = 1600,           //!< OpenNI2 (for Kinect)
    CAP_OPENNI2_ASUS = 1610,      //!< OpenNI2 (for Asus Xtion and Occipital Structure sensors)
    CAP_OPENNI2_ASTRA = 1620,     //!< OpenNI2 (for Orbbec Astra)
    CAP_GPHOTO2 = 1700,           //!< gPhoto2 connection
    CAP_GSTREAMER = 1800,         //!< GStreamer
    CAP_FFMPEG = 1900,            //!< Open and record video file or stream using the FFMPEG library
    CAP_IMAGES = 2000,            //!< OpenCV Image Sequence (e.g. img_%02d.jpg)
    CAP_ARAVIS = 2100,            //!< Aravis SDK
    CAP_OPENCV_MJPEG = 2200,      //!< Built-in OpenCV MotionJPEG codec
    CAP_INTEL_MFX = 2300,         //!< Intel MediaSDK
    CAP_XINE = 2400,              //!< XINE engine (Linux)
    CAP_UEYE = 2500,              //!< uEye Camera API
};

class CvCapture_FFMPEG_proxy : public IVideoCapture {
public:
    CvCapture_FFMPEG_proxy() { ffmpegCapture = 0; }
    CvCapture_FFMPEG_proxy(const std::string& filename, const VideoCaptureParameters& params) : ffmpegCapture(NULL) { open(filename, params); }
    virtual ~CvCapture_FFMPEG_proxy() { close(); }

    virtual double getProperty(int propId) const override { return ffmpegCapture ? cvGetCaptureProperty_FFMPEG(ffmpegCapture, propId) : 0; }
    virtual bool setProperty(int propId, double value) override { return ffmpegCapture ? cvSetCaptureProperty_FFMPEG(ffmpegCapture, propId, value) != 0 : false; }
    virtual bool grabFrame() override { return ffmpegCapture ? cvGrabFrame_FFMPEG(ffmpegCapture) != 0 : false; }
    virtual bool retrieveFrame(int, IVideoCaptureFrame& frame, bool rgb) override {
        unsigned char* data = 0;
        int step = 0, width = 0, height = 0, cn = 0;

        if (!ffmpegCapture) return false;

        if (!cvRetrieveFrame_FFMPEG(ffmpegCapture, &data, &step, &width, &height, &cn, rgb)) return false;

        frame.width = width;
        frame.height = height;
        frame.data = data;
        frame.step = step;
        frame.cn = cn;

        return true;
    }
    bool open(const std::string& filename, const VideoCaptureParameters& params) {
        close();

        ffmpegCapture = cvCreateFileCaptureWithParams_FFMPEG(filename.c_str(), params);
        return ffmpegCapture != 0;
    }
    void close() {
        if (ffmpegCapture) cvReleaseCapture_FFMPEG(&ffmpegCapture);
        assert(ffmpegCapture == 0);
        ffmpegCapture = 0;
    }

    virtual void seek(int64_t frame_number) override {
        if (ffmpegCapture) {
            ffmpegCapture->seek(frame_number);
        }
    }

    virtual void seek(double sec) override {
        if (ffmpegCapture) {
            ffmpegCapture->seek(sec);
        }
    }

    virtual bool isOpened() const override { return ffmpegCapture != 0; }

protected:
    CvCapture_FFMPEG* ffmpegCapture;
};

IVideoCapture* cvCreateFileCapture_FFMPEG_proxy(const std::string& filename, const VideoCaptureParameters& params) {
    IVideoCapture* capture = new CvCapture_FFMPEG_proxy(filename, params);
    if (capture && capture->isOpened()) return capture;
    delete capture;
    return nullptr;
}
