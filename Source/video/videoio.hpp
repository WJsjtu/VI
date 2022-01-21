#pragma once

enum VideoCaptureProperties {
    CAP_PROP_POS_MSEC = 0,       //!< Current position of the video file in milliseconds.
    CAP_PROP_POS_FRAMES = 1,     //!< 0-based index of the frame to be decoded/captured next.
    CAP_PROP_POS_AVI_RATIO = 2,  //!< Relative position of the video file: 0=start of the film, 1=end of the film.
    CAP_PROP_FRAME_WIDTH = 3,    //!< Width of the frames in the video stream.
    CAP_PROP_FRAME_HEIGHT = 4,   //!< Height of the frames in the video stream.
    CAP_PROP_FPS = 5,            //!< Frame rate.
    CAP_PROP_FOURCC = 6,         //!< 4-character code of codec. see VideoWriter::fourcc .
    CAP_PROP_FRAME_COUNT = 7,    //!< Number of frames in the video file.
    CAP_PROP_FORMAT = 8,         //!< Format of the %Mat objects (see Mat::type()) returned by VideoCapture::retrieve().
    //!< Set value -1 to fetch undecoded RAW video streams (as Mat 8UC1).
    CAP_PROP_MODE = 9,          //!< Backend-specific value indicating the current capture mode.
    CAP_PROP_BRIGHTNESS = 10,   //!< Brightness of the image (only for those cameras that support).
    CAP_PROP_CONTRAST = 11,     //!< Contrast of the image (only for cameras).
    CAP_PROP_SATURATION = 12,   //!< Saturation of the image (only for cameras).
    CAP_PROP_HUE = 13,          //!< Hue of the image (only for cameras).
    CAP_PROP_GAIN = 14,         //!< Gain of the image (only for those cameras that support).
    CAP_PROP_EXPOSURE = 15,     //!< Exposure (only for those cameras that support).
    CAP_PROP_CONVERT_RGB = 16,  //!< Boolean flags indicating whether images should be converted to RGB. <br/>
    //!< *GStreamer note*: The flag is ignored in case if custom pipeline is used. It's user responsibility to interpret pipeline output.

    CAP_PROP_WHITE_BALANCE_BLUE_U = 17,  //!< Currently unsupported.
    CAP_PROP_RECTIFICATION = 18,         //!< Rectification flag for stereo cameras (note: only supported by DC1394 v 2.x backend currently).
    CAP_PROP_MONOCHROME = 19,
    CAP_PROP_SHARPNESS = 20,
    CAP_PROP_AUTO_EXPOSURE = 21,  //!< DC1394: exposure control done by camera, user can adjust reference level using this feature.
    CAP_PROP_GAMMA = 22,
    CAP_PROP_TEMPERATURE = 23,
    CAP_PROP_TRIGGER = 24,
    CAP_PROP_TRIGGER_DELAY = 25,
    CAP_PROP_WHITE_BALANCE_RED_V = 26,
    CAP_PROP_ZOOM = 27,
    CAP_PROP_FOCUS = 28,
    CAP_PROP_GUID = 29,
    CAP_PROP_ISO_SPEED = 30,
    CAP_PROP_BACKLIGHT = 32,
    CAP_PROP_PAN = 33,
    CAP_PROP_TILT = 34,
    CAP_PROP_ROLL = 35,
    CAP_PROP_IRIS = 36,
    CAP_PROP_SETTINGS = 37,  //!< Pop up video/camera filter dialog (note: only supported by DSHOW backend currently. The property value is ignored)
    CAP_PROP_BUFFERSIZE = 38,
    CAP_PROP_AUTOFOCUS = 39,
    CAP_PROP_SAR_NUM = 40,             //!< Sample aspect ratio: num/den (num)
    CAP_PROP_SAR_DEN = 41,             //!< Sample aspect ratio: num/den (den)
    CAP_PROP_BACKEND = 42,             //!< Current backend (enum VideoCaptureAPIs). Read-only property
    CAP_PROP_CHANNEL = 43,             //!< Video input or Channel Number (only for those cameras that support)
    CAP_PROP_AUTO_WB = 44,             //!< enable/ disable auto white-balance
    CAP_PROP_WB_TEMPERATURE = 45,      //!< white-balance color temperature
    CAP_PROP_CODEC_PIXEL_FORMAT = 46,  //!< (read-only) codec's pixel format. 4-character code - see VideoWriter::fourcc . Subset of [AV_PIX_FMT_*](https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/raw.c) or -1 if unknown
    CAP_PROP_BITRATE = 47,             //!< (read-only) Video bitrate in kbits/s
    CAP_PROP_ORIENTATION_META = 48,    //!< (read-only) Frame rotation defined by stream meta (applicable for FFmpeg back-end only)
    CAP_PROP_ORIENTATION_AUTO = 49,    //!< if true - rotates output frames of CvCapture considering video file's metadata  (applicable for FFmpeg back-end only) (https://github.com/opencv/opencv/issues/15499)
    CAP_PROP_HW_ACCELERATION = 50,     //!< (**open-only**) Hardware acceleration type (see #VideoAccelerationType). Setting supported only via `params` parameter in cv::VideoCapture constructor / .open() method. Default value is backend-specific.
    CAP_PROP_HW_DEVICE = 51,           //!< (**open-only**) Hardware device index (select GPU if multiple available). Device enumeration is acceleration type specific.
    CAP_PROP_HW_ACCELERATION_USE_OPENCL =
        52,                               //!< (**open-only**) If non-zero, create new OpenCL context and bind it to current thread. The OpenCL context created with Video Acceleration context attached it (if not attached yet) for optimized GPU data copy between HW accelerated decoder and cv::UMat.
    CAP_PROP_OPEN_TIMEOUT_MSEC = 53,      //!< (**open-only**) timeout in milliseconds for opening a video capture (applicable for FFmpeg back-end only)
    CAP_PROP_READ_TIMEOUT_MSEC = 54,      //!< (**open-only**) timeout in milliseconds for reading from a video capture (applicable for FFmpeg back-end only)
    CAP_PROP_STREAM_OPEN_TIME_USEC = 55,  //<! (read-only) time in microseconds since Jan 1 1970 when stream was opened. Applicable for FFmpeg backend only. Useful for RTSP and other live streams
    CAP_PROP_VIDEO_TOTAL_CHANNELS = 56,   //!< (read-only) Number of video channels
    CAP_PROP_VIDEO_STREAM = 57,           //!< (**open-only**) Specify video stream, 0-based index. Use -1 to disable video stream from file or IP cameras. Default value is 0.
    CAP_PROP_AUDIO_STREAM = 58,           //!< (**open-only**) Specify stream in multi-language media files, -1 - disable audio processing or microphone. Default value is -1.
    CAP_PROP_AUDIO_POS = 59,              //!< (read-only) Audio position is measured in samples. Accurate audio sample timestamp of previous grabbed fragment. See CAP_PROP_AUDIO_SAMPLES_PER_SECOND and CAP_PROP_AUDIO_SHIFT_NSEC.
    CAP_PROP_AUDIO_SHIFT_NSEC =
        60,  //!< (read only) Contains the time difference between the start of the audio stream and the video stream in nanoseconds. Positive value means that audio is started after the first video frame. Negative value means that audio is started before the first video frame.
    CAP_PROP_AUDIO_DATA_DEPTH = 61,          //!< (open, read) Alternative definition to bits-per-sample, but with clear handling of 32F / 32S
    CAP_PROP_AUDIO_SAMPLES_PER_SECOND = 62,  //!< (read-only) determined from file/codec input. If not specified, then selected audio sample rate is 44100
    CAP_PROP_AUDIO_BASE_INDEX = 63,          //!< (read-only) Index of the first audio channel for .retrieve() calls. That audio channel number continues enumeration after video channels.
    CAP_PROP_AUDIO_TOTAL_CHANNELS = 64,      //!< (read-only) Number of audio channels in the selected audio stream (mono, stereo, etc)
    CAP_PROP_AUDIO_TOTAL_STREAMS = 65,       //!< (read-only) Number of audio streams.
    CAP_PROP_AUDIO_SYNCHRONIZE = 66,         //!< (open, read) Enables audio synchronization.
#ifndef CV_DOXYGEN
    CV__CAP_PROP_LATEST
#endif
};

enum VideoAccelerationType {
    VIDEO_ACCELERATION_NONE = 0,  //!< Do not require any specific H/W acceleration, prefer software processing.
                                  //!< Reading of this value means that special H/W accelerated handling is not added or not detected by OpenCV.

    VIDEO_ACCELERATION_ANY = 1,  //!< Prefer to use H/W acceleration. If no one supported, then fallback to software processing.
    //!< @note H/W acceleration may require special configuration of used environment.
    //!< @note Results in encoding scenario may differ between software and hardware accelerated encoders.

    VIDEO_ACCELERATION_D3D11 = 2,  //!< DirectX 11
    VIDEO_ACCELERATION_VAAPI = 3,  //!< VAAPI
    VIDEO_ACCELERATION_MFX = 4,    //!< libmfx (Intel MediaSDK/oneVPL)
};
