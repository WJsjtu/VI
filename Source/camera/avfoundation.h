#ifdef __APPLE__
#pragma once
#import <AVFoundation/AVFoundation.h>
#include <Availability.h>
#import <Foundation/Foundation.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <thread>

struct IplImage {
    int nChannels;
    int rowBytes;
    int width;
    int height;
    char* imageData;
};

/********************** Declaration of class headers ************************/

/*****************************************************************************
 *
 * CaptureDelegate Declaration.
 *
 * CaptureDelegate is notified on a separate thread by the OS whenever there
 *   is a new frame. When "updateImage" is called from the main thread, it
 *   copies this new frame into an IplImage, but only if this frame has not
 *   been copied before. When "getOutput" is called from the main thread,
 *   it gives the last copied IplImage.
 *
 *****************************************************************************/

@interface CaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
    std::mutex newFrameMutex;
    bool mHasNewFrame;
    CVPixelBufferRef mGrabbedPixels;
    CVImageBufferRef mCurrentImageBuffer;
    IplImage* mDeviceImage;
    uint8_t* mOutImageData;
    uint8_t* mOutImageDataFlip;
    size_t currSize;
}

- (void)captureOutput:(AVCaptureOutput*)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection*)connection;

- (BOOL)grabImageUntilDate;
- (int)updateImage;
- (void)getOutput:(uint8_t*)buffer forRGB:(bool)rgb forFlipX:(bool)flipX forFlipY:(bool)flipY;

@end

/*****************************************************************************
 *
 * CvCaptureCAM Declaration.
 *
 * CvCaptureCAM is the instantiation of a capture source for cameras.
 *
 *****************************************************************************/

enum PROPERTY { CV_CAP_PROP_FRAME_WIDTH, CV_CAP_PROP_FRAME_HEIGHT, CV_CAP_PROP_FPS };

class videoInput {
public:
    static std::vector<std::string> getDeviceList();
    videoInput(int cameraNum = -1);
    ~videoInput();
    bool grabFrame();
    void retrieveFrame(uint8_t* buffer, bool rgb, bool flipX, bool flipY);
    double getProperty(PROPERTY property_id) const;
    bool setProperty(PROPERTY property_id, double value);

    int didStart();

private:
    AVCaptureSession* mCaptureSession;
    AVCaptureDeviceInput* mCaptureDeviceInput;
    AVCaptureVideoDataOutput* mCaptureVideoDataOutput;
    AVCaptureDevice* mCaptureDevice;
    CaptureDelegate* mCapture;

    int startCaptureDevice(int cameraNum);
    void stopCaptureDevice();

    void setWidthHeight();
    bool grabFrame(double timeOut);

    int camNum;
    int width;
    int height;
    int settingWidth;
    int settingHeight;

    int started;
};
#endif
