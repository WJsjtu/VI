#ifdef __APPLE__
/*M///////////////////////////////////////////////////////////////////////////////////////
 //
 // IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 //
 // By downloading, copying, installing or using the software you agree to this
 license.
 // If you do not agree to this license, do not download, install,
 // copy or use the software.
 //
 //
 //                          License Agreement
 //                For Open Source Computer Vision Library
 //
 // Copyright (C) 2013, OpenCV Foundation, all rights reserved.
 // Third party copyrights are property of their respective owners.
 //
 // Redistribution and use in source and binary forms, with or without
 modification,
 // are permitted provided that the following conditions are met:
 //
 // * Redistribution's of source code must retain the above copyright notice,
 // this list of conditions and the following disclaimer.
 //
 // * Redistribution's in binary form must reproduce the above copyright notice,
 // this list of conditions and the following disclaimer in the documentation
 // and/or other materials provided with the distribution.
 //
 // * The name of the copyright holders may not be used to endorse or promote
 products
 // derived from this software without specific prior written permission.
 //
 // This software is provided by the copyright holders and contributors "as is"
 and
 // any express or implied warranties, including, but not limited to, the
 implied
 // warranties of merchantability and fitness for a particular purpose are
 disclaimed.
 // In no event shall the contributor be liable for any direct,
 // indirect, incidental, special, exemplary, or consequential damages
 // (including, but not limited to, procurement of substitute goods or services;
 // loss of use, data, or profits; or business interruption) however caused
 // and on any theory of liability, whether in contract, strict liability,
 // or tort (including negligence or otherwise) arising in any way out of
 // the use of this software, even if advised of the possibility of such damage.
 //
 //M*////////////////////////////////////////////////////////////////////////////////////////

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include "avfoundation.h"
#include <Accelerate/Accelerate.h>
#include "utils.h"

#include <stdio.h>
#include <ostream>
#include <sstream>

IplImage *cvCreateImage(int width, int height, int channels) {
    IplImage *img = (IplImage *)malloc(sizeof(IplImage));
    memset(img, 0, sizeof(IplImage));
    img->nChannels = channels;
    img->width = width;
    img->height = height;
    return img;
}

void cvReleaseImage(IplImage **image) {
    if (image && *image) {
        IplImage *img = *image;
        *image = 0;
        img->imageData = 0;
        *image = 0;
        free(img);
    }
}

/********************** Implementation of Classes ****************************/

/*****************************************************************************
 *
 * CvCaptureCAM Implementation.
 *
 * CvCaptureCAM is the instantiation of a capture source for cameras.
 *
 *****************************************************************************/

videoInput::videoInput(int cameraNum) {
    mCaptureSession = nil;
    mCaptureDeviceInput = nil;
    mCaptureVideoDataOutput = nil;
    mCaptureDevice = nil;
    mCapture = nil;
    
    width = 0;
    height = 0;
    settingWidth = 0;
    settingHeight = 0;
    
    camNum = cameraNum;
    
    if (!startCaptureDevice(camNum)) {
        Utils::LoggerPrintf(Utils::LogLevel::Error, "Camera failed to properly initialize!\n");
        started = 0;
    } else {
        started = 1;
    }
}

videoInput::~videoInput() { stopCaptureDevice(); }

int videoInput::didStart() { return started; }

bool videoInput::grabFrame() { return grabFrame(1); }

bool videoInput::grabFrame(double timeOut) {
    NSAutoreleasePool *localpool = [[NSAutoreleasePool alloc] init];
    
    bool isGrabbed = false;
    NSDate *limit = [NSDate dateWithTimeIntervalSinceNow:timeOut];
    if ([mCapture grabImageUntilDate:limit]) {
        [mCapture updateImage];
        isGrabbed = true;
    }
    
    [localpool drain];
    return isGrabbed;
}

void videoInput::retrieveFrame(uint8_t* buffer) { return [mCapture getOutput:buffer] ; }

void videoInput::stopCaptureDevice() {
    NSAutoreleasePool *localpool = [[NSAutoreleasePool alloc] init];
    
    [mCaptureSession stopRunning];
    
    [mCaptureSession release];
    [mCaptureDeviceInput release];
    // [mCaptureDevice release]; fix #7833
    
    [mCaptureVideoDataOutput release];
    [mCapture release];
    
    [localpool drain];
}

std::vector<std::string> videoInput::getDeviceList() {
    NSAutoreleasePool *localpool = [[NSAutoreleasePool alloc] init];
    std::vector<std::string> result;
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
    if (@available(macOS 10.14, *)) {
        AVAuthorizationStatus status =
        [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
        if (status == AVAuthorizationStatusDenied) {
            Utils::LoggerPrintf(Utils::LogLevel::Error,
                                "Camera access has been denied. Either run 'tccutil reset "
                                "Camera' "
                                "command in same terminal to reset application authorization status, "
                                "either modify 'System Preferences -> Security & Privacy -> Camera' "
                                "settings for your application.\n");
            [localpool drain];
            return result;
        } else if (status != AVAuthorizationStatusAuthorized) {
            Utils::LoggerPrintf(Utils::LogLevel::Warning,
                                "Not authorized to capture video (status %ld), "
                                "requesting...\n",
                                status);
            [AVCaptureDevice
             requestAccessForMediaType:AVMediaTypeVideo
             completionHandler:^(BOOL){/* we don't care */}];
            if ([NSThread isMainThread]) {
                // we run the main loop for 0.1 sec to show the message
                [[NSRunLoop mainRunLoop]
                 runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
            } else {
                Utils::LoggerPrintf(Utils::LogLevel::Error,
                                    "Can not spin main run loop from other thread, set "
                                    "OPENCV_AVFOUNDATION_SKIP_AUTH=1 to disable authorization "
                                    "request "
                                    "and perform it in your application.\n");
                [localpool drain];
                return result;
            }
        }
    }
#endif
    
    // get capture device
    NSArray *devices = [[AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]
                        arrayByAddingObjectsFromArray:[AVCaptureDevice
                                                       devicesWithMediaType:AVMediaTypeMuxed]];
    
    if (devices.count == 0) {
        Utils::LoggerPrintf(Utils::LogLevel::Error,
                            "AVFoundation didn't find any attached Video Input Devices!\n");
        [localpool drain];
        return result;
    }
    
    for (int i = 0; NSUInteger(i) < devices.count; i++) {
        AVCaptureDevice* device = devices[i];
        if(device) {
            std::string localizedName = [device.localizedName UTF8String];
            result.push_back(localizedName);
        } else {
            result.push_back("");
        }
    }
    
    [localpool drain];
    return result;
}

int videoInput::startCaptureDevice(int cameraNum) {
    NSAutoreleasePool *localpool = [[NSAutoreleasePool alloc] init];
    
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
    if (@available(macOS 10.14, *)) {
        AVAuthorizationStatus status =
        [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
        if (status == AVAuthorizationStatusDenied) {
            Utils::LoggerPrintf(Utils::LogLevel::Error,
                                "Camera access has been denied. Either run 'tccutil reset "
                                "Camera' "
                                "command in same terminal to reset application authorization status, "
                                "either modify 'System Preferences -> Security & Privacy -> Camera' "
                                "settings for your application.\n");
            [localpool drain];
            return 0;
        } else if (status != AVAuthorizationStatusAuthorized) {
            Utils::LoggerPrintf(Utils::LogLevel::Warning,
                                "Not authorized to capture video (status %ld), "
                                "requesting...\n",
                                status);
            [AVCaptureDevice
             requestAccessForMediaType:AVMediaTypeVideo
             completionHandler:^(BOOL){/* we don't care */}];
            if ([NSThread isMainThread]) {
                // we run the main loop for 0.1 sec to show the message
                [[NSRunLoop mainRunLoop]
                 runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
            } else {
                Utils::LoggerPrintf(Utils::LogLevel::Error,
                                    "Can not spin main run loop from other thread, set "
                                    "OPENCV_AVFOUNDATION_SKIP_AUTH=1 to disable authorization "
                                    "request "
                                    "and perform it in your application.\n");
                [localpool drain];
                return 0;
            }
        }
    }
#endif
    
    // get capture device
    NSArray *devices = [[AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]
                        arrayByAddingObjectsFromArray:[AVCaptureDevice
                                                       devicesWithMediaType:AVMediaTypeMuxed]];
    
    if (devices.count == 0) {
        Utils::LoggerPrintf(Utils::LogLevel::Error,
                            "AVFoundation didn't find any attached Video Input Devices!\n");
        [localpool drain];
        return 0;
    }
    
    if (cameraNum < 0 || devices.count <= NSUInteger(cameraNum)) {
        Utils::LoggerPrintf(Utils::LogLevel::Error, "Out device of bound (0-%ld): %d\n",
                            devices.count - 1, cameraNum);
        [localpool drain];
        return 0;
    }
    
    mCaptureDevice = devices[cameraNum];
    
    if (!mCaptureDevice) {
        Utils::LoggerPrintf(Utils::LogLevel::Error, "Device %d not able to use.\n", cameraNum);
        [localpool drain];
        return 0;
    }
    
    // get input device
    NSError *error = nil;
    mCaptureDeviceInput =
    [[AVCaptureDeviceInput alloc] initWithDevice:mCaptureDevice error:&error];
    if (error) {
        Utils::LoggerPrintf(Utils::LogLevel::Error, "Error in [AVCaptureDeviceInput initWithDevice:error:]\n");
        NSLog(@"OpenCV: %@", error.localizedDescription);
        [localpool drain];
        return 0;
    }
    
    // create output
    mCapture = [[CaptureDelegate alloc] init];
    mCaptureVideoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    dispatch_queue_t queue =
    dispatch_queue_create("cameraQueue", DISPATCH_QUEUE_SERIAL);
    [mCaptureVideoDataOutput setSampleBufferDelegate:mCapture queue:queue];
    dispatch_release(queue);
    
    OSType pixelFormat = kCVPixelFormatType_32BGRA;
    // OSType pixelFormat = kCVPixelFormatType_422YpCbCr8;
    NSDictionary *pixelBufferOptions;
    if (width > 0 && height > 0) {
        pixelBufferOptions = @{
            (id)kCVPixelBufferWidthKey : @(1.0 * width),
            (id)kCVPixelBufferHeightKey : @(1.0 * height),
            (id)kCVPixelBufferPixelFormatTypeKey : @(pixelFormat)
        };
    } else {
        pixelBufferOptions =
        @{(id)kCVPixelBufferPixelFormatTypeKey : @(pixelFormat)};
    }
    mCaptureVideoDataOutput.videoSettings = pixelBufferOptions;
    mCaptureVideoDataOutput.alwaysDiscardsLateVideoFrames = YES;
    
    // create session
    mCaptureSession = [[AVCaptureSession alloc] init];
    mCaptureSession.sessionPreset = AVCaptureSessionPresetHigh;
    [mCaptureSession addInput:mCaptureDeviceInput];
    [mCaptureSession addOutput:mCaptureVideoDataOutput];
    
    [mCaptureSession startRunning];
    
    // flush old position image
    grabFrame(1);
    
    [localpool drain];
    return 1;
}

void videoInput::setWidthHeight() {
    NSMutableDictionary *pixelBufferOptions =
    [mCaptureVideoDataOutput.videoSettings mutableCopy];
    
    while (true) {
        // auto matching
        pixelBufferOptions[(id)kCVPixelBufferWidthKey] = @(1.0 * width);
        pixelBufferOptions[(id)kCVPixelBufferHeightKey] = @(1.0 * height);
        mCaptureVideoDataOutput.videoSettings = pixelBufferOptions;
        
        // compare matched size and my options
        CMFormatDescriptionRef format =
        mCaptureDevice.activeFormat.formatDescription;
        CMVideoDimensions deviceSize =
        CMVideoFormatDescriptionGetDimensions(format);
        if (deviceSize.width == width && deviceSize.height == height) {
            break;
        }
        
        // fit my options to matched size
        width = deviceSize.width;
        height = deviceSize.height;
    }
    
    // flush old size image
    grabFrame(1);
    
    [pixelBufferOptions release];
}

double videoInput::getProperty(PROPERTY property_id) const {
    NSAutoreleasePool *localpool = [[NSAutoreleasePool alloc] init];
    
    CMFormatDescriptionRef format = mCaptureDevice.activeFormat.formatDescription;
    CMVideoDimensions s1 = CMVideoFormatDescriptionGetDimensions(format);
    double retval = 0;
    
    switch (property_id) {
        case CV_CAP_PROP_FRAME_WIDTH:
            retval = s1.width;
            break;
        case CV_CAP_PROP_FRAME_HEIGHT:
            retval = s1.height;
            break;
        case CV_CAP_PROP_FPS: {
            CMTime frameDuration = mCaptureDevice.activeVideoMaxFrameDuration;
            retval = frameDuration.timescale / double(frameDuration.value);
        } break;
        default:
            break;
    }
    
    [localpool drain];
    return retval;
}

bool videoInput::setProperty(PROPERTY property_id, double value) {
    NSAutoreleasePool *localpool = [[NSAutoreleasePool alloc] init];
    
    bool isSucceeded = false;
    
    switch (property_id) {
        case CV_CAP_PROP_FRAME_WIDTH:
            width = value;
            settingWidth = 1;
            if (settingWidth && settingHeight) {
                setWidthHeight();
                settingWidth = 0;
                settingHeight = 0;
            }
            isSucceeded = true;
            break;
        case CV_CAP_PROP_FRAME_HEIGHT:
            height = value;
            settingHeight = 1;
            if (settingWidth && settingHeight) {
                setWidthHeight();
                settingWidth = 0;
                settingHeight = 0;
            }
            isSucceeded = true;
            break;
        case CV_CAP_PROP_FPS:
            if ([mCaptureDevice lockForConfiguration:NULL]) {
                NSArray *ranges =
                mCaptureDevice.activeFormat.videoSupportedFrameRateRanges;
                AVFrameRateRange *matchedRange = ranges[0];
                double minDiff = fabs(matchedRange.maxFrameRate - value);
                for (AVFrameRateRange *range in ranges) {
                    double diff = fabs(range.maxFrameRate - value);
                    if (diff < minDiff) {
                        minDiff = diff;
                        matchedRange = range;
                    }
                }
                mCaptureDevice.activeVideoMinFrameDuration =
                matchedRange.minFrameDuration;
                mCaptureDevice.activeVideoMaxFrameDuration =
                matchedRange.minFrameDuration;
                isSucceeded = true;
                [mCaptureDevice unlockForConfiguration];
            }
            break;
        default:
            break;
    }
    
    [localpool drain];
    return isSucceeded;
}

/*****************************************************************************
 *
 * CaptureDelegate Implementation.
 *
 * CaptureDelegate is notified on a separate thread by the OS whenever there
 *   is a new frame. When "updateImage" is called from the main thread, it
 *   copies this new frame into an IplImage, but only if this frame has not
 *   been copied before. When "getOutput" is called from the main thread,
 *   it gives the last copied IplImage.
 *
 *****************************************************************************/

@implementation CaptureDelegate

- (id)init {
    [super init];
    mFrameNumber = 0;
    mLastFrameNumber = -1;
    mCurrentImageBuffer = NULL;
    mGrabbedPixels = NULL;
    mDeviceImage = NULL;
    mDeviceImageFlipX = NULL;
    currSize = 0;
    return self;
}

- (void)dealloc {
    free(mDeviceImageFlipX);
    cvReleaseImage(&mDeviceImage);
    CVBufferRelease(mCurrentImageBuffer);
    CVBufferRelease(mGrabbedPixels);
    [super dealloc];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection {
    (void)captureOutput;
    (void)sampleBuffer;
    (void)connection;
    
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    CVBufferRetain(imageBuffer);
    
    CVBufferRelease(mCurrentImageBuffer);
    mCurrentImageBuffer = imageBuffer;
    mFrameNumber++;
}

- (void)getOutput:(uint8_t *)buffer {
    vImage_Buffer src_buffer;
    src_buffer.data = mDeviceImageFlipX;
    src_buffer.width = mDeviceImage->width;
    src_buffer.height = mDeviceImage->height;
    src_buffer.rowBytes = mDeviceImage->rowBytes;
    
    vImage_Buffer dst_buffer;
    dst_buffer.data = buffer;
    dst_buffer.width = mDeviceImage->width;
    dst_buffer.height = mDeviceImage->height;
    dst_buffer.rowBytes = mDeviceImage->width * 3;
    
    vImageConvert_BGRA8888toRGB888(&src_buffer, &dst_buffer, kvImageDoNotTile);
}

- (BOOL)grabImageUntilDate:(NSDate *)limit {
    BOOL isGrabbed = NO;
    
    if (mGrabbedPixels) {
        CVBufferRelease(mGrabbedPixels);
    }
    if (mLastFrameNumber != mFrameNumber) {
        mLastFrameNumber = mFrameNumber;
        isGrabbed = YES;
        mGrabbedPixels = CVBufferRetain(mCurrentImageBuffer);
    }
    
    return isGrabbed;
}

- (int)updateImage {
    if (!mGrabbedPixels) {
        return 0;
    }
    
    CVPixelBufferLockBaseAddress(mGrabbedPixels, 0);
    void *baseaddress = CVPixelBufferGetBaseAddress(mGrabbedPixels);
    
    size_t width = CVPixelBufferGetWidth(mGrabbedPixels);
    size_t height = CVPixelBufferGetHeight(mGrabbedPixels);
    size_t rowBytes = CVPixelBufferGetBytesPerRow(mGrabbedPixels);
    OSType pixelFormat = CVPixelBufferGetPixelFormatType(mGrabbedPixels);
    
    if (rowBytes == 0) {
        Utils::LoggerPrintf(Utils::LogLevel::Error, "Error: rowBytes == 0\n");
        CVPixelBufferUnlockBaseAddress(mGrabbedPixels, 0);
        CVBufferRelease(mGrabbedPixels);
        mGrabbedPixels = NULL;
        return 0;
    }
    
    if (currSize != width * 4 * height) {
        currSize = width * 4 * height;
        free(mDeviceImageFlipX);
        mDeviceImageFlipX = reinterpret_cast<uint8_t *>(malloc(currSize));
    }
    
    if (pixelFormat == kCVPixelFormatType_32BGRA) {
        if (mDeviceImage == NULL) {
            mDeviceImage = cvCreateImage(int(width), int(height), 4);
        }
        mDeviceImage->width = int(width);
        mDeviceImage->height = int(height);
        mDeviceImage->nChannels = 4;
        mDeviceImage->rowBytes = rowBytes;
        mDeviceImage->imageData = reinterpret_cast<char *>(baseaddress);
        
        vImage_Buffer src_buffer;
        src_buffer.data = mDeviceImage->imageData;
        src_buffer.width = width;
        src_buffer.height = height;
        src_buffer.rowBytes = rowBytes;
        
        vImage_Buffer src_buffer_flipX;
        src_buffer_flipX.data = mDeviceImageFlipX;
        src_buffer_flipX.width = width;
        src_buffer_flipX.height = height;
        src_buffer_flipX.rowBytes = rowBytes;
        
        vImageHorizontalReflect_ARGB8888(&src_buffer, &src_buffer_flipX, kvImageNoFlags);
        
    } else {
        Utils::LoggerPrintf(Utils::LogLevel::Error, "Unknown pixel format 0x%08X\n", pixelFormat);
        CVPixelBufferUnlockBaseAddress(mGrabbedPixels, 0);
        CVBufferRelease(mGrabbedPixels);
        mGrabbedPixels = NULL;
        return 0;
    }
    
    CVPixelBufferUnlockBaseAddress(mGrabbedPixels, 0);
    CVBufferRelease(mGrabbedPixels);
    mGrabbedPixels = NULL;
    
    return 1;
}

@end
#endif

