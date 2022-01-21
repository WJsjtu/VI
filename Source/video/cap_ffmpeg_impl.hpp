#include "cap_ffmpeg_legacy_api.hpp"
#include "utils.h"
#include <assert.h>
#include <algorithm>
#include <limits>
#include "videoio.hpp"

#ifndef __OPENCV_BUILD
#define CV_FOURCC(c1, c2, c3, c4) (((c1)&255) + (((c2)&255) << 8) + (((c3)&255) << 16) + (((c4)&255) << 24))
#endif

#define CALC_FFMPEG_VERSION(a, b, c) (a << 16 | b << 8 | c)

#if defined _MSC_VER && _MSC_VER >= 1200
#pragma warning(disable : 4244 4510 4610)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4996)  // was declared deprecated
#endif

#ifndef CV_UNUSED  // Required for standalone compilation mode (OpenCV defines this in base.hpp)
#define CV_UNUSED(name) (void)name
#endif

#include "ffmpeg_codecs.hpp"
extern "C" {
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>

#if LIBAVUTIL_BUILD >= (LIBAVUTIL_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(51, 63, 100) : CALC_FFMPEG_VERSION(54, 6, 0))
#include <libavutil/imgutils.h>
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

// GCC 4.x compilation bug. Details: https://github.com/opencv/opencv/issues/20292
#if (defined(__GNUC__) && __GNUC__ < 5) && !defined(__clang__)
#undef USE_AV_HW_CODECS
#define USE_AV_HW_CODECS 0
#endif

//#define USE_AV_HW_CODECS 0
#ifndef USE_AV_HW_CODECS
#if LIBAVUTIL_VERSION_MAJOR >= 56  // FFMPEG 4.0+
#define USE_AV_HW_CODECS 1
#include "cap_ffmpeg_hw.hpp"
#else
#define USE_AV_HW_CODECS 0
#endif
#endif

#if defined _MSC_VER && _MSC_VER >= 1200
#pragma warning(default : 4244 4510 4610)
#endif

#if defined _WIN32
#include <windows.h>
#elif defined __linux__ || defined __APPLE__ || defined __HAIKU__
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#if defined __APPLE__
#include <sys/sysctl.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#endif

#if defined(__APPLE__)
#define AV_NOPTS_VALUE_ ((int64_t)0x8000000000000000LL)
#else
#define AV_NOPTS_VALUE_ ((int64_t)AV_NOPTS_VALUE)
#endif

#ifndef AVERROR_EOF
#define AVERROR_EOF (-MKTAG('E', 'O', 'F', ' '))
#endif

#if LIBAVCODEC_BUILD >= CALC_FFMPEG_VERSION(54, 25, 0)
#define CV_CODEC_ID AVCodecID
#define CV_CODEC(name) AV_##name
#else
#define CV_CODEC_ID CodecID
#define CV_CODEC(name) name
#endif

#ifndef PKT_FLAG_KEY
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

#if LIBAVUTIL_BUILD >= (LIBAVUTIL_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(52, 38, 100) : CALC_FFMPEG_VERSION(52, 13, 0))
#define USE_AV_FRAME_GET_BUFFER 1
#else
#define USE_AV_FRAME_GET_BUFFER 0
#ifndef AV_NUM_DATA_POINTERS  // required for 0.7.x/0.8.x ffmpeg releases
#define AV_NUM_DATA_POINTERS 4
#endif
#endif

#ifndef USE_AV_INTERRUPT_CALLBACK
#define USE_AV_INTERRUPT_CALLBACK 1
#endif

#ifndef USE_AV_SEND_FRAME_API
// https://github.com/FFmpeg/FFmpeg/commit/7fc329e2dd6226dfecaa4a1d7adf353bf2773726
#if LIBAVCODEC_VERSION_MICRO >= 100 && LIBAVCODEC_BUILD >= CALC_FFMPEG_VERSION(57, 37, 100)
#define USE_AV_SEND_FRAME_API 1
#else
#define USE_AV_SEND_FRAME_API 0
#endif
#endif

#if USE_AV_INTERRUPT_CALLBACK
#define LIBAVFORMAT_INTERRUPT_OPEN_DEFAULT_TIMEOUT_MS 30000
#define LIBAVFORMAT_INTERRUPT_READ_DEFAULT_TIMEOUT_MS 30000

#ifdef _WIN32
// http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows

static inline LARGE_INTEGER get_filetime_offset() {
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return t;
}

static inline void get_monotonic_time(timespec* tv) {
    LARGE_INTEGER t;
    FILETIME f;
    double microseconds;
    static LARGE_INTEGER offset;
    static double frequencyToMicroseconds;
    static int initialized = 0;
    static BOOL usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        } else {
            offset = get_filetime_offset();
            frequencyToMicroseconds = 10.;
        }
    }

    if (usePerformanceCounter) {
        QueryPerformanceCounter(&t);
    } else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = (LONGLONG)microseconds;
    tv->tv_sec = t.QuadPart / 1000000;
    tv->tv_nsec = (t.QuadPart % 1000000) * 1000;
}
#else
static inline void get_monotonic_time(timespec* time) {
#if defined(__APPLE__) && defined(__MACH__)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    time->tv_sec = mts.tv_sec;
    time->tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_MONOTONIC, time);
#endif
}
#endif

static inline timespec get_monotonic_time_diff(timespec start, timespec end) {
    timespec temp;
    if (end.tv_nsec - start.tv_nsec < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

static inline double get_monotonic_time_diff_ms(timespec time1, timespec time2) {
    timespec delta = get_monotonic_time_diff(time1, time2);
    double milliseconds = delta.tv_sec * 1000 + (double)delta.tv_nsec / 1000000.0;

    return milliseconds;
}
#endif  // USE_AV_INTERRUPT_CALLBACK

static int get_number_of_cpus(void) {
#if defined _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    return (int)sysinfo.dwNumberOfProcessors;
#elif defined __linux__ || defined __HAIKU__
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined __APPLE__
    int numCPU = 0;
    int mib[4];
    size_t len = sizeof(numCPU);

    // set the mib for hw.ncpu
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    // get the number of CPUs from the system
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    if (numCPU < 1) {
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &numCPU, &len, NULL, 0);

        if (numCPU < 1) numCPU = 1;
    }

    return (int)numCPU;
#else
    return 1;
#endif
}

struct Image_FFMPEG {
    unsigned char* data;
    int step;
    int width;
    int height;
    int cn;
};

#if USE_AV_INTERRUPT_CALLBACK
struct AVInterruptCallbackMetadata {
    timespec value;
    unsigned int timeout_after_ms;
    int timeout;
};

// https://github.com/opencv/opencv/pull/12693#issuecomment-426236731
static inline const char* _opencv_avcodec_get_name(CV_CODEC_ID id) {
#if LIBAVCODEC_VERSION_MICRO >= 100 && LIBAVCODEC_BUILD >= CALC_FFMPEG_VERSION(53, 47, 100)
    return avcodec_get_name(id);
#else
    const AVCodecDescriptor* cd;
    AVCodec* codec;

    if (id == AV_CODEC_ID_NONE) {
        return "none";
    }
    cd = avcodec_descriptor_get(id);
    if (cd) {
        return cd->name;
    }
    codec = avcodec_find_decoder(id);
    if (codec) {
        return codec->name;
    }
    codec = avcodec_find_encoder(id);
    if (codec) {
        return codec->name;
    }

    return "unknown_codec";
#endif
}

static inline int _opencv_ffmpeg_interrupt_callback(void* ptr) {
    AVInterruptCallbackMetadata* metadata = (AVInterruptCallbackMetadata*)ptr;
    assert(metadata);

    if (metadata->timeout_after_ms == 0) {
        return 0;  // timeout is disabled
    }

    timespec now;
    get_monotonic_time(&now);

    metadata->timeout = get_monotonic_time_diff_ms(metadata->value, now) > metadata->timeout_after_ms;

    return metadata->timeout ? -1 : 0;
}
#endif

static inline void _opencv_ffmpeg_av_packet_unref(AVPacket* pkt) {
#if LIBAVCODEC_BUILD >= (LIBAVCODEC_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(55, 25, 100) : CALC_FFMPEG_VERSION(55, 16, 0))
    av_packet_unref(pkt);
#else
    av_free_packet(pkt);
#endif
};

static inline void _opencv_ffmpeg_av_image_fill_arrays(void* frame, uint8_t* ptr, enum AVPixelFormat pix_fmt, int width, int height) {
#if LIBAVUTIL_BUILD >= (LIBAVUTIL_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(51, 63, 100) : CALC_FFMPEG_VERSION(54, 6, 0))
    av_image_fill_arrays(((AVFrame*)frame)->data, ((AVFrame*)frame)->linesize, ptr, pix_fmt, width, height, 1);
#else
    avpicture_fill((AVPicture*)frame, ptr, pix_fmt, width, height);
#endif
};

static inline int _opencv_ffmpeg_av_image_get_buffer_size(enum AVPixelFormat pix_fmt, int width, int height) {
#if LIBAVUTIL_BUILD >= (LIBAVUTIL_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(51, 63, 100) : CALC_FFMPEG_VERSION(54, 6, 0))
    return av_image_get_buffer_size(pix_fmt, width, height, 1);
#else
    return avpicture_get_size(pix_fmt, width, height);
#endif
};

static AVRational _opencv_ffmpeg_get_sample_aspect_ratio(AVStream* stream) {
#if LIBAVUTIL_VERSION_MICRO >= 100 && LIBAVUTIL_BUILD >= CALC_FFMPEG_VERSION(54, 5, 100)
    return av_guess_sample_aspect_ratio(NULL, stream, NULL);
#else
    AVRational undef = {0, 1};

    // stream
    AVRational ratio = stream ? stream->sample_aspect_ratio : undef;
    av_reduce(&ratio.num, &ratio.den, ratio.num, ratio.den, INT_MAX);
    if (ratio.num > 0 && ratio.den > 0) return ratio;

    // codec
    ratio = stream && stream->codec ? stream->codec->sample_aspect_ratio : undef;
    av_reduce(&ratio.num, &ratio.den, ratio.num, ratio.den, INT_MAX);
    if (ratio.num > 0 && ratio.den > 0) return ratio;

    return undef;
#endif
}

struct CvCapture_FFMPEG {
    bool open(const char* filename, const VideoCaptureParameters& params);
    void close();

    double getProperty(int) const;
    bool setProperty(int, double);
    bool grabFrame();
    bool retrieveFrame(int, unsigned char** data, int* step, int* width, int* height, int* cn, bool rgb);

    void init();

    void seek(int64_t frame_number);
    void seek(double sec);
    bool slowSeek(int framenumber);

    int64_t get_total_frames() const;
    double get_duration_sec() const;
    double get_fps() const;
    int64_t get_bitrate() const;

    double r2d(AVRational r) const;
    int64_t dts_to_frame_number(int64_t dts);
    double dts_to_sec(int64_t dts) const;
    void get_rotation_angle();

    AVFormatContext* ic;
    AVCodec* avcodec;
    int video_stream;
    AVStream* video_st;
    AVFrame* picture;
    AVFrame rgb_picture;
    int64_t picture_pts;

    AVPacket packet;
    Image_FFMPEG frame;
    struct SwsContext* img_convert_ctx;

    int64_t frame_number, first_frame_number;

    bool rotation_auto;
    int rotation_angle;  // valid 0, 90, 180, 270
    double eps_zero;
    /*
       'filename' contains the filename of the videosource,
       'filename==NULL' indicates that ffmpeg's seek support works
       for the particular file.
       'filename!=NULL' indicates that the slow fallback function is used for seeking,
       and so the filename is needed to reopen the file on backward seeking.
    */
    char* filename;

    AVDictionary* dict;
#if USE_AV_INTERRUPT_CALLBACK
    int open_timeout;
    int read_timeout;
    AVInterruptCallbackMetadata interrupt_metadata;
#endif

    bool setRaw();
    bool processRawPacket();
    bool rawMode;
    bool rawModeInitialized;
    AVPacket packet_filtered;
#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(58, 20, 100)
    AVBSFContext* bsfc;
#else
    AVBitStreamFilterContext* bsfc;
#endif
    VideoAccelerationType va_type;
    int hw_device;
    int use_opencl;
};

void CvCapture_FFMPEG::init() {
    ic = 0;
    video_stream = -1;
    video_st = 0;
    picture = 0;
    picture_pts = AV_NOPTS_VALUE_;
    first_frame_number = -1;
    memset(&rgb_picture, 0, sizeof(rgb_picture));
    memset(&frame, 0, sizeof(frame));
    filename = 0;
    memset(&packet, 0, sizeof(packet));
    av_init_packet(&packet);
    img_convert_ctx = 0;

    avcodec = 0;
    frame_number = 0;
    eps_zero = 0.000025;

    rotation_angle = 0;

#if (LIBAVUTIL_BUILD >= CALC_FFMPEG_VERSION(52, 92, 100))
    rotation_auto = true;
#else
    rotation_auto = false;
#endif
    dict = NULL;

#if USE_AV_INTERRUPT_CALLBACK
    open_timeout = LIBAVFORMAT_INTERRUPT_OPEN_DEFAULT_TIMEOUT_MS;
    read_timeout = LIBAVFORMAT_INTERRUPT_READ_DEFAULT_TIMEOUT_MS;
#endif

    rawMode = false;
    rawModeInitialized = false;
    memset(&packet_filtered, 0, sizeof(packet_filtered));
    av_init_packet(&packet_filtered);
    bsfc = NULL;
    va_type = VIDEO_ACCELERATION_NONE;  // TODO OpenCV 5.0: change to _ANY?
    hw_device = -1;
    use_opencl = 0;
}

void CvCapture_FFMPEG::close() {
    if (img_convert_ctx) {
        sws_freeContext(img_convert_ctx);
        img_convert_ctx = 0;
    }

    if (picture) {
#if LIBAVCODEC_BUILD >= (LIBAVCODEC_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(55, 45, 101) : CALC_FFMPEG_VERSION(55, 28, 1))
        av_frame_free(&picture);
#elif LIBAVCODEC_BUILD >= (LIBAVCODEC_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(54, 59, 100) : CALC_FFMPEG_VERSION(54, 28, 0))
        avcodec_free_frame(&picture);
#else
        av_free(picture);
#endif
    }

    if (video_st) {
        avcodec_close(video_st->codec);
        video_st = NULL;
    }

    if (ic) {
        avformat_close_input(&ic);
        ic = NULL;
    }

#if USE_AV_FRAME_GET_BUFFER
    av_frame_unref(&rgb_picture);
#else
    if (rgb_picture.data[0]) {
        free(rgb_picture.data[0]);
        rgb_picture.data[0] = 0;
    }
#endif

    // free last packet if exist
    if (packet.data) {
        _opencv_ffmpeg_av_packet_unref(&packet);
        packet.data = NULL;
    }

    if (dict != NULL) av_dict_free(&dict);

    if (packet_filtered.data) {
        _opencv_ffmpeg_av_packet_unref(&packet_filtered);
        packet_filtered.data = NULL;
    }

    if (bsfc) {
#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(58, 20, 100)
        av_bsf_free(&bsfc);
#else
        av_bitstream_filter_close(bsfc);
#endif
    }

    init();
}

#ifndef AVSEEK_FLAG_FRAME
#define AVSEEK_FLAG_FRAME 0
#endif
#ifndef AVSEEK_FLAG_ANY
#define AVSEEK_FLAG_ANY 1
#endif

#if defined(__OPENCV_BUILD) || defined(BUILD_PLUGIN)
typedef cv::Mutex ImplMutex;
#else
class ImplMutex {
public:
    ImplMutex() { init(); }
    ~ImplMutex() { destroy(); }

    void init();
    void destroy();

    void lock();
    bool trylock();
    void unlock();

    struct Impl;

protected:
    Impl* impl;

private:
    ImplMutex(const ImplMutex&);
    ImplMutex& operator=(const ImplMutex& m);
};

#if defined _WIN32 || defined WINCE

struct ImplMutex::Impl {
    void init() {
#if (_WIN32_WINNT >= 0x0600)
        ::InitializeCriticalSectionEx(&cs, 1000, 0);
#else
        ::InitializeCriticalSection(&cs);
#endif
        refcount = 1;
    }
    void destroy() { DeleteCriticalSection(&cs); }

    void lock() { EnterCriticalSection(&cs); }
    bool trylock() { return TryEnterCriticalSection(&cs) != 0; }
    void unlock() { LeaveCriticalSection(&cs); }

    CRITICAL_SECTION cs;
    int refcount;
};

#elif defined __APPLE__

#include <libkern/OSAtomic.h>

struct ImplMutex::Impl {
    void init() {
        sl = OS_SPINLOCK_INIT;
        refcount = 1;
    }
    void destroy() {}

    void lock() { OSSpinLockLock(&sl); }
    bool trylock() { return OSSpinLockTry(&sl); }
    void unlock() { OSSpinLockUnlock(&sl); }

    OSSpinLock sl;
    int refcount;
};

#elif defined __linux__ && !defined __ANDROID__

struct ImplMutex::Impl {
    void init() {
        pthread_spin_init(&sl, 0);
        refcount = 1;
    }
    void destroy() { pthread_spin_destroy(&sl); }

    void lock() { pthread_spin_lock(&sl); }
    bool trylock() { return pthread_spin_trylock(&sl) == 0; }
    void unlock() { pthread_spin_unlock(&sl); }

    pthread_spinlock_t sl;
    int refcount;
};

#else

struct ImplMutex::Impl {
    void init() {
        pthread_mutex_init(&sl, 0);
        refcount = 1;
    }
    void destroy() { pthread_mutex_destroy(&sl); }

    void lock() { pthread_mutex_lock(&sl); }
    bool trylock() { return pthread_mutex_trylock(&sl) == 0; }
    void unlock() { pthread_mutex_unlock(&sl); }

    pthread_mutex_t sl;
    int refcount;
};

#endif

void ImplMutex::init() {
    impl = new Impl();
    impl->init();
}
void ImplMutex::destroy() {
    impl->destroy();
    delete (impl);
    impl = NULL;
}
void ImplMutex::lock() { impl->lock(); }
void ImplMutex::unlock() { impl->unlock(); }
bool ImplMutex::trylock() { return impl->trylock(); }

class AutoLock {
public:
    AutoLock(ImplMutex& m) : mutex(&m) { mutex->lock(); }
    ~AutoLock() { mutex->unlock(); }

protected:
    ImplMutex* mutex;

private:
    AutoLock(const AutoLock&);             // disabled
    AutoLock& operator=(const AutoLock&);  // disabled
};
#endif

static ImplMutex _mutex;

static int LockCallBack(void** mutex, AVLockOp op) {
    ImplMutex* localMutex = reinterpret_cast<ImplMutex*>(*mutex);
    switch (op) {
        case AV_LOCK_CREATE:
            localMutex = new ImplMutex();
            if (!localMutex) return 1;
            *mutex = localMutex;
            if (!*mutex) return 1;
            break;

        case AV_LOCK_OBTAIN: localMutex->lock(); break;

        case AV_LOCK_RELEASE: localMutex->unlock(); break;

        case AV_LOCK_DESTROY:
            delete localMutex;
            localMutex = NULL;
            *mutex = NULL;
            break;
    }
    return 0;
}

static void ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vargs) {
    static bool skip_header = false;
    static int prev_level = -1;
    CV_UNUSED(ptr);
    if (level > av_log_get_level()) return;
    if (!skip_header || level != prev_level) printf("[OPENCV:FFMPEG:%02d] ", level);
    vprintf(fmt, vargs);
    size_t fmt_len = strlen(fmt);
    skip_header = fmt_len > 0 && fmt[fmt_len - 1] != '\n';
    prev_level = level;
}

class InternalFFMpegRegister {
public:
    static void init() {
        AutoLock lock(_mutex);
        static InternalFFMpegRegister instance;
        initLogger_();  // update logger setup unconditionally (GStreamer's libav plugin may override these settings)
    }
    static void initLogger_() {
#ifndef NO_GETENV
        char* debug_option = getenv("OPENCV_FFMPEG_DEBUG");
        char* level_option = getenv("OPENCV_FFMPEG_LOGLEVEL");
        int level = AV_LOG_VERBOSE;
        if (level_option != NULL) {
            level = atoi(level_option);
        }
        if ((debug_option != NULL) || (level_option != NULL)) {
            av_log_set_level(level);
            av_log_set_callback(ffmpeg_log_callback);
        } else
#endif
        {
            av_log_set_level(AV_LOG_ERROR);
        }
    }

public:
    InternalFFMpegRegister() {
        avformat_network_init();

        /* register all codecs, demux and protocols */
        av_register_all();

        /* register a callback function for synchronization */
        av_lockmgr_register(&LockCallBack);
    }
    ~InternalFFMpegRegister() {
        av_lockmgr_register(NULL);
        av_log_set_callback(NULL);
    }
};

bool CvCapture_FFMPEG::open(const char* _filename, const VideoCaptureParameters& params) {
    InternalFFMpegRegister::init();

    AutoLock lock(_mutex);

    unsigned i;
    bool valid = false;

    close();

    if (!params.empty()) {
        if (params.has(CAP_PROP_FORMAT)) {
            int value = params.get<int>(CAP_PROP_FORMAT);
            if (value == -1) {
                CV_LOG_INFO(NULL, "VIDEOIO/FFMPEG: enabled demuxer only mode: '" << (_filename ? _filename : "<NULL>") << "'");
                rawMode = true;
            } else {
                CV_LOG_ERROR(NULL, "VIDEOIO/FFMPEG: CAP_PROP_FORMAT parameter value is invalid/unsupported: " << value);
                return false;
            }
        }
        if (params.has(CAP_PROP_HW_ACCELERATION)) {
            va_type = params.get<VideoAccelerationType>(CAP_PROP_HW_ACCELERATION);
#if !USE_AV_HW_CODECS
            if (va_type != VIDEO_ACCELERATION_NONE && va_type != VIDEO_ACCELERATION_ANY) {
                CV_LOG_ERROR(NULL, "VIDEOIO/FFMPEG: FFmpeg backend is build without acceleration support. Can't handle CAP_PROP_HW_ACCELERATION parameter. Bailout");
                return false;
            }
#endif
        }
        if (params.has(CAP_PROP_HW_DEVICE)) {
            hw_device = params.get<int>(CAP_PROP_HW_DEVICE);
            if (va_type == VIDEO_ACCELERATION_NONE && hw_device != -1) {
                CV_LOG_ERROR(NULL, "VIDEOIO/FFMPEG: Invalid usage of CAP_PROP_HW_DEVICE without requested H/W acceleration. Bailout");
                return false;
            }
            if (va_type == VIDEO_ACCELERATION_ANY && hw_device != -1) {
                CV_LOG_ERROR(NULL, "VIDEOIO/FFMPEG: Invalid usage of CAP_PROP_HW_DEVICE with 'ANY' H/W acceleration. Bailout");
                return false;
            }
        }
        if (params.has(CAP_PROP_HW_ACCELERATION_USE_OPENCL)) {
            use_opencl = params.get<int>(CAP_PROP_HW_ACCELERATION_USE_OPENCL);
        }
#if USE_AV_INTERRUPT_CALLBACK
        if (params.has(CAP_PROP_OPEN_TIMEOUT_MSEC)) {
            open_timeout = params.get<int>(CAP_PROP_OPEN_TIMEOUT_MSEC);
        }
        if (params.has(CAP_PROP_READ_TIMEOUT_MSEC)) {
            read_timeout = params.get<int>(CAP_PROP_READ_TIMEOUT_MSEC);
        }
#endif
        if (params.warnUnusedParameters()) {
            CV_LOG_ERROR(NULL, "VIDEOIO/FFMPEG: unsupported parameters in .open(), see logger INFO channel for details. Bailout");
            return false;
        }
    }

#if USE_AV_INTERRUPT_CALLBACK
    /* interrupt callback */
    interrupt_metadata.timeout_after_ms = open_timeout;
    get_monotonic_time(&interrupt_metadata.value);

    ic = avformat_alloc_context();
    ic->interrupt_callback.callback = _opencv_ffmpeg_interrupt_callback;
    ic->interrupt_callback.opaque = &interrupt_metadata;
#endif

#ifndef NO_GETENV
    //	char* options = getenv("OPENCV_FFMPEG_CAPTURE_OPTIONS");
    //	if (options == NULL)
    //	{
    av_dict_set(&dict, "rtsp_transport", "tcp", 0);
    //	}
    //	else
    //	{
    //#if LIBAVUTIL_BUILD >= (LIBAVUTIL_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(52, 17, 100) : CALC_FFMPEG_VERSION(52, 7, 0))
    //		av_dict_parse_string(&dict, options, ";", "|", 0);
    //#else
    //		av_dict_set(&dict, "rtsp_transport", "tcp", 0);
    //#endif
    //	}
#else
    av_dict_set(&dict, "rtsp_transport", "tcp", 0);
#endif
    AVInputFormat* input_format = NULL;
    AVDictionaryEntry* entry = av_dict_get(dict, "input_format", NULL, 0);
    if (entry != 0) {
        input_format = av_find_input_format(entry->value);
    }

    int err = avformat_open_input(&ic, _filename, input_format, &dict);

    if (err < 0) {
        CV_LOG_WARN(NULL, "Error opening file");
        CV_LOG_WARN(NULL, _filename);
        goto exit_func;
    }
    err = avformat_find_stream_info(ic, NULL);
    if (err < 0) {
        CV_LOG_WARN(NULL, "Could not find codec parameters");
        goto exit_func;
    }
    for (i = 0; i < ic->nb_streams; i++) {
        AVCodecContext* enc = ic->streams[i]->codec;

        //#ifdef FF_API_THREAD_INIT
        //        avcodec_thread_init(enc, get_number_of_cpus());
        //#else
        enc->thread_count = get_number_of_cpus();
        //#endif

        AVDictionaryEntry* avdiscard_entry = av_dict_get(dict, "avdiscard", NULL, 0);

        if (avdiscard_entry) {
            if (strcmp(avdiscard_entry->value, "all") == 0)
                enc->skip_frame = AVDISCARD_ALL;
            else if (strcmp(avdiscard_entry->value, "bidir") == 0)
                enc->skip_frame = AVDISCARD_BIDIR;
            else if (strcmp(avdiscard_entry->value, "default") == 0)
                enc->skip_frame = AVDISCARD_DEFAULT;
            else if (strcmp(avdiscard_entry->value, "none") == 0)
                enc->skip_frame = AVDISCARD_NONE;
                // NONINTRA flag was introduced with version bump at revision:
                // https://github.com/FFmpeg/FFmpeg/commit/b152152df3b778d0a86dcda5d4f5d065b4175a7b
                // This key is supported only for FFMPEG version
#if LIBAVCODEC_VERSION_MICRO >= 100 && LIBAVCODEC_BUILD >= CALC_FFMPEG_VERSION(55, 67, 100)
            else if (strcmp(avdiscard_entry->value, "nonintra") == 0)
                enc->skip_frame = AVDISCARD_NONINTRA;
#endif
            else if (strcmp(avdiscard_entry->value, "nonkey") == 0)
                enc->skip_frame = AVDISCARD_NONKEY;
            else if (strcmp(avdiscard_entry->value, "nonref") == 0)
                enc->skip_frame = AVDISCARD_NONREF;
        }

        if (AVMEDIA_TYPE_VIDEO == enc->codec_type && video_stream < 0) {
            CV_LOG_DEBUG(NULL, "FFMPEG: stream[" << i << "] is video stream with codecID=" << (int)enc->codec_id << " width=" << enc->width << " height=" << enc->height);

            // backup encoder' width/height
            int enc_width = enc->width;
            int enc_height = enc->height;

#if !USE_AV_HW_CODECS
            va_type = VIDEO_ACCELERATION_NONE;
#endif

            // find and open decoder, try HW acceleration types specified in 'hw_acceleration' list (in order)
            AVCodec* codec = NULL;
            err = -1;
#if USE_AV_HW_CODECS
            HWAccelIterator accel_iter(va_type, false /*isEncoder*/, dict);
            while (accel_iter.good()) {
#else
            do {
#endif
#if USE_AV_HW_CODECS
                accel_iter.parse_next();
                AVHWDeviceType hw_type = accel_iter.hw_type();
                enc->get_format = avcodec_default_get_format;
                if (enc->hw_device_ctx) {
                    av_buffer_unref(&enc->hw_device_ctx);
                }
                if (hw_type != AV_HWDEVICE_TYPE_NONE) {
                    CV_LOG_DEBUG(NULL, "FFMPEG: trying to configure H/W acceleration: '" << accel_iter.hw_type_device_string() << "'");
                    AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
                    codec = hw_find_codec(enc->codec_id, hw_type, av_codec_is_decoder, accel_iter.disabled_codecs().c_str(), &hw_pix_fmt);
                    if (codec) {
                        if (hw_pix_fmt != AV_PIX_FMT_NONE) enc->get_format = hw_get_format_callback;  // set callback to select HW pixel format, not SW format
                        enc->hw_device_ctx = hw_create_device(hw_type, hw_device, accel_iter.device_subname(), use_opencl != 0);
                        if (!enc->hw_device_ctx) {
                            CV_LOG_DEBUG(NULL, "FFMPEG: ... can't create H/W device: '" << accel_iter.hw_type_device_string() << "'");
                            codec = NULL;
                        }
                    }
                } else if (hw_type == AV_HWDEVICE_TYPE_NONE)
#endif  // USE_AV_HW_CODECS
                {
                    AVDictionaryEntry* video_codec_param = av_dict_get(dict, "video_codec", NULL, 0);
                    if (video_codec_param == NULL) {
                        codec = avcodec_find_decoder(enc->codec_id);
                        if (!codec) {
                            CV_LOG_ERROR(NULL, "Could not find decoder for codec_id=" << (int)enc->codec_id);
                        }
                    } else {
                        CV_LOG_DEBUG(NULL, "FFMPEG: Using video_codec='" << video_codec_param->value << "'");
                        codec = avcodec_find_decoder_by_name(video_codec_param->value);
                        if (!codec) {
                            CV_LOG_ERROR(NULL, "Could not find decoder '" << video_codec_param->value << "'");
                        }
                    }
                }
                if (!codec) continue;
                err = avcodec_open2(enc, codec, NULL);
                if (err >= 0) {
#if USE_AV_HW_CODECS
                    va_type = hw_type_to_va_type(hw_type);
                    if (hw_type != AV_HWDEVICE_TYPE_NONE && hw_device < 0) hw_device = 0;
#endif
                    break;
                } else {
                    CV_LOG_ERROR(NULL, "Could not open codec " << codec->name << ", error: " << err);
                }
#if USE_AV_HW_CODECS
            }  // while (accel_iter.good())
#else
            } while (0);
#endif
            if (err < 0) {
                CV_LOG_ERROR(NULL, "VIDEOIO/FFMPEG: Failed to initialize VideoCapture");
                goto exit_func;
            }

            // checking width/height (since decoder can sometimes alter it, eg. vp6f)
            if (enc_width && (enc->width != enc_width)) enc->width = enc_width;
            if (enc_height && (enc->height != enc_height)) enc->height = enc_height;

            video_stream = i;
            video_st = ic->streams[i];
#if LIBAVCODEC_BUILD >= (LIBAVCODEC_VERSION_MICRO >= 100 ? CALC_FFMPEG_VERSION(55, 45, 101) : CALC_FFMPEG_VERSION(55, 28, 1))
            picture = av_frame_alloc();
#else
            picture = avcodec_alloc_frame();
#endif

            frame.width = enc->width;
            frame.height = enc->height;
            frame.cn = 3;
            frame.step = 0;
            frame.data = NULL;
            get_rotation_angle();
            break;
        }
    }

    if (video_stream >= 0) valid = true;

exit_func:

#if USE_AV_INTERRUPT_CALLBACK
    // deactivate interrupt callback
    interrupt_metadata.timeout_after_ms = 0;
#endif

    if (!valid) close();
    return valid;
}

bool CvCapture_FFMPEG::setRaw() {
    if (!rawMode) {
        if (frame_number != 0) {
            CV_LOG_WARN(NULL, "Incorrect usage: do not grab frames before .set(CAP_PROP_FORMAT, -1)");
        }
        // binary stream filter creation is moved into processRawPacket()
        rawMode = true;
    }
    return true;
}

bool CvCapture_FFMPEG::processRawPacket() {
    if (packet.data == NULL)  // EOF
        return false;
    if (!rawModeInitialized) {
        rawModeInitialized = true;
#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(58, 20, 100)
        CV_CODEC_ID eVideoCodec = ic->streams[video_stream]->codecpar->codec_id;
#else
        CV_CODEC_ID eVideoCodec = video_st->codec->codec_id;
#endif
        const char* filterName = NULL;
        if (eVideoCodec == CV_CODEC(CODEC_ID_H264)
#if LIBAVCODEC_VERSION_MICRO >= 100 && LIBAVCODEC_BUILD >= CALC_FFMPEG_VERSION(57, 24, 102)  // FFmpeg 3.0
            || eVideoCodec == CV_CODEC(CODEC_ID_H265)
#elif LIBAVCODEC_VERSION_MICRO < 100 && LIBAVCODEC_BUILD >= CALC_FFMPEG_VERSION(55, 34, 1)  // libav v10+
            || eVideoCodec == CV_CODEC(CODEC_ID_HEVC)
#endif
        ) {
            // check start code prefixed mode (as defined in the Annex B H.264 / H.265 specification)
            if (packet.size >= 5 && !(packet.data[0] == 0 && packet.data[1] == 0 && packet.data[2] == 0 && packet.data[3] == 1) && !(packet.data[0] == 0 && packet.data[1] == 0 && packet.data[2] == 1)) {
                filterName = eVideoCodec == CV_CODEC(CODEC_ID_H264) ? "h264_mp4toannexb" : "hevc_mp4toannexb";
            }
        }
        if (filterName) {
#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(58, 20, 100)
            const AVBitStreamFilter* bsf = av_bsf_get_by_name(filterName);
            if (!bsf) {
                CV_LOG_WARN(NULL, "Bitstream filter is not available: " << filterName);
                return false;
            }
            int err = av_bsf_alloc(bsf, &bsfc);
            if (err < 0) {
                CV_LOG_WARN(NULL, "Error allocating context for bitstream buffer");
                return false;
            }
            avcodec_parameters_copy(bsfc->par_in, ic->streams[video_stream]->codecpar);
            err = av_bsf_init(bsfc);
            if (err < 0) {
                CV_LOG_WARN(NULL, "Error initializing bitstream buffer");
                return false;
            }
#else
            bsfc = av_bitstream_filter_init(filterName);
            if (!bsfc) {
                CV_LOG_WARN(cv::format("Bitstream filter is not available: %s", filterName).c_str());
                return false;
            }
#endif
        }
    }
    if (bsfc) {
        if (packet_filtered.data) {
            _opencv_ffmpeg_av_packet_unref(&packet_filtered);
        }

#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(58, 20, 100)
        int err = av_bsf_send_packet(bsfc, &packet);
        if (err < 0) {
            CV_LOG_WARN(NULL, "Packet submission for filtering failed");
            return false;
        }
        err = av_bsf_receive_packet(bsfc, &packet_filtered);
        if (err < 0) {
            CV_LOG_WARN(NULL, "Filtered packet retrieve failed");
            return false;
        }
#else
        AVCodecContext* ctx = ic->streams[video_stream]->codec;
        int err = av_bitstream_filter_filter(bsfc, ctx, NULL, &packet_filtered.data, &packet_filtered.size, packet.data, packet.size, packet_filtered.flags & AV_PKT_FLAG_KEY);
        if (err < 0) {
            CV_LOG_WARN("Packet filtering failed");
            return false;
        }
#endif
        return packet_filtered.data != NULL;
    }
    return packet.data != NULL;
}

bool CvCapture_FFMPEG::grabFrame() {
    bool valid = false;

    int count_errs = 0;
    const int max_number_of_attempts = 1 << 9;

    if (!ic || !video_st) return false;

    if (ic->streams[video_stream]->nb_frames > 0 && frame_number > ic->streams[video_stream]->nb_frames) return false;

    picture_pts = AV_NOPTS_VALUE_;

#if USE_AV_INTERRUPT_CALLBACK
    // activate interrupt callback
    get_monotonic_time(&interrupt_metadata.value);
    interrupt_metadata.timeout_after_ms = read_timeout;
#endif

#if USE_AV_SEND_FRAME_API
    // check if we can receive frame from previously decoded packet
    valid = avcodec_receive_frame(video_st->codec, picture) >= 0;
#endif

    // get the next frame
    while (!valid) {
        _opencv_ffmpeg_av_packet_unref(&packet);

#if USE_AV_INTERRUPT_CALLBACK
        if (interrupt_metadata.timeout) {
            valid = false;
            break;
        }
#endif

        int ret = av_read_frame(ic, &packet);

        if (ret == AVERROR(EAGAIN)) continue;

        if (ret == AVERROR_EOF) {
            if (rawMode) break;

            // flush cached frames from video decoder
            packet.data = NULL;
            packet.size = 0;
            packet.stream_index = video_stream;
        }

        if (packet.stream_index != video_stream) {
            _opencv_ffmpeg_av_packet_unref(&packet);
            count_errs++;
            if (count_errs > max_number_of_attempts) break;
            continue;
        }

        if (rawMode) {
            valid = processRawPacket();
            break;
        }

        // Decode video frame
#if USE_AV_SEND_FRAME_API
        if (avcodec_send_packet(video_st->codec, &packet) < 0) {
            break;
        }
        ret = avcodec_receive_frame(video_st->codec, picture);
#else
        int got_picture = 0;
        avcodec_decode_video2(video_st->codec, picture, &got_picture, &packet);
        ret = got_picture ? 0 : -1;
#endif
        if (ret >= 0) {
            // picture_pts = picture->best_effort_timestamp;
            if (picture_pts == AV_NOPTS_VALUE_) picture_pts = picture->pkt_pts != AV_NOPTS_VALUE_ && picture->pkt_pts != 0 ? picture->pkt_pts : picture->pkt_dts;

            valid = true;
        } else if (ret == AVERROR(EAGAIN)) {
            continue;
        } else {
            count_errs++;
            if (count_errs > max_number_of_attempts) break;
        }
    }

    if (valid) frame_number++;

    if (!rawMode && valid && first_frame_number < 0) first_frame_number = dts_to_frame_number(picture_pts);

#if USE_AV_INTERRUPT_CALLBACK
    // deactivate interrupt callback
    interrupt_metadata.timeout_after_ms = 0;
#endif

    // return if we have a new frame or not
    return valid;
}

bool CvCapture_FFMPEG::retrieveFrame(int, unsigned char** data, int* step, int* width, int* height, int* cn, bool rgb) {
    if (!video_st) return false;

    if (rawMode) {
        AVPacket& p = bsfc ? packet_filtered : packet;
        *data = p.data;
        *step = p.size;
        *width = p.size;
        *height = 1;
        *cn = 1;
        return p.data != NULL;
    }

    AVFrame* sw_picture = picture;
#if USE_AV_HW_CODECS
    // if hardware frame, copy it to system memory
    if (picture && picture->hw_frames_ctx) {
        sw_picture = av_frame_alloc();
        // if (av_hwframe_map(sw_picture, picture, AV_HWFRAME_MAP_READ) < 0) {
        if (av_hwframe_transfer_data(sw_picture, picture, 0) < 0) {
            CV_LOG_ERROR(NULL, "Error copying data from GPU to CPU (av_hwframe_transfer_data)");
            return false;
        }
    }
#endif

    if (!sw_picture || !sw_picture->data[0]) return false;

    if (img_convert_ctx == NULL || frame.width != video_st->codec->width || frame.height != video_st->codec->height || frame.data == NULL) {
        // Some sws_scale optimizations have some assumptions about alignment of data/step/width/height
        // Also we use coded_width/height to workaround problem with legacy ffmpeg versions (like n0.8)
        int buffer_width = video_st->codec->coded_width, buffer_height = video_st->codec->coded_height;

        img_convert_ctx = sws_getCachedContext(img_convert_ctx, buffer_width, buffer_height, (AVPixelFormat)sw_picture->format, buffer_width, buffer_height, rgb ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

        if (img_convert_ctx == NULL) return false;  // CV_Error(0, "Cannot initialize the conversion context!");

#if USE_AV_FRAME_GET_BUFFER
        av_frame_unref(&rgb_picture);
        rgb_picture.format = rgb ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_BGR24;
        rgb_picture.width = buffer_width;
        rgb_picture.height = buffer_height;
        if (0 != av_frame_get_buffer(&rgb_picture, 1)) {
            CV_LOG_WARN(NULL, "OutOfMemory");
            return false;
        }
#else
        int aligns[AV_NUM_DATA_POINTERS];
        avcodec_align_dimensions2(video_st->codec, &buffer_width, &buffer_height, aligns);
        rgb_picture.data[0] = (uint8_t*)realloc(rgb_picture.data[0], _opencv_ffmpeg_av_image_get_buffer_size(AV_PIX_FMT_BGR24, buffer_width, buffer_height));
        _opencv_ffmpeg_av_image_fill_arrays(&rgb_picture, rgb_picture.data[0], AV_PIX_FMT_BGR24, buffer_width, buffer_height);
#endif
        frame.width = video_st->codec->width;
        frame.height = video_st->codec->height;
        frame.cn = 3;
        frame.data = rgb_picture.data[0];
        frame.step = rgb_picture.linesize[0];
    }

    sws_scale(img_convert_ctx, sw_picture->data, sw_picture->linesize, 0, video_st->codec->coded_height, rgb_picture.data, rgb_picture.linesize);

    *data = frame.data;
    *step = frame.step;
    *width = frame.width;
    *height = frame.height;
    *cn = frame.cn;

#if USE_AV_HW_CODECS
    if (sw_picture != picture) {
        av_frame_unref(sw_picture);
    }
#endif
    return true;
}

double CvCapture_FFMPEG::getProperty(int property_id) const {
    if (!video_st) return 0;

    double codec_tag = 0;
    CV_CODEC_ID codec_id = AV_CODEC_ID_NONE;
    const char* codec_fourcc = NULL;

    switch (property_id) {
        case CAP_PROP_POS_MSEC:
            if (picture_pts == AV_NOPTS_VALUE_) {
                return 0;
            }
            return (dts_to_sec(picture_pts) * 1000);
        case CAP_PROP_POS_FRAMES: return (double)frame_number;
        case CAP_PROP_POS_AVI_RATIO: return r2d(ic->streams[video_stream]->time_base);
        case CAP_PROP_FRAME_COUNT: return (double)get_total_frames();
        case CAP_PROP_FRAME_WIDTH: return (double)((rotation_auto && ((rotation_angle % 180) != 0)) ? frame.height : frame.width);
        case CAP_PROP_FRAME_HEIGHT: return (double)((rotation_auto && ((rotation_angle % 180) != 0)) ? frame.width : frame.height);
        case CAP_PROP_FPS: return get_fps();
        case CAP_PROP_FOURCC:
            codec_id = video_st->codec->codec_id;
            codec_tag = (double)video_st->codec->codec_tag;

            if (codec_tag || codec_id == AV_CODEC_ID_NONE) {
                return codec_tag;
            }

            codec_fourcc = _opencv_avcodec_get_name(codec_id);
            if (!codec_fourcc || strlen(codec_fourcc) < 4 || strcmp(codec_fourcc, "unknown_codec") == 0) {
                return codec_tag;
            }

            return (double)CV_FOURCC(codec_fourcc[0], codec_fourcc[1], codec_fourcc[2], codec_fourcc[3]);
        case CAP_PROP_SAR_NUM: return _opencv_ffmpeg_get_sample_aspect_ratio(ic->streams[video_stream]).num;
        case CAP_PROP_SAR_DEN: return _opencv_ffmpeg_get_sample_aspect_ratio(ic->streams[video_stream]).den;
        case CAP_PROP_CODEC_PIXEL_FORMAT: {
            AVPixelFormat pix_fmt = video_st->codec->pix_fmt;
            unsigned int fourcc_tag = avcodec_pix_fmt_to_codec_tag(pix_fmt);
            return (fourcc_tag == 0) ? (double)-1 : (double)fourcc_tag;
        }
        case CAP_PROP_FORMAT:
            if (rawMode) return -1;
            break;
        case CAP_PROP_BITRATE: return static_cast<double>(get_bitrate());
        case CAP_PROP_ORIENTATION_META: return static_cast<double>(rotation_angle);
        case CAP_PROP_ORIENTATION_AUTO:
#if LIBAVUTIL_BUILD >= CALC_FFMPEG_VERSION(52, 94, 100)
            return static_cast<double>(rotation_auto);
#else
            return 0;
#endif
#if USE_AV_HW_CODECS
        case CAP_PROP_HW_ACCELERATION: return static_cast<double>(va_type);
        case CAP_PROP_HW_DEVICE: return static_cast<double>(hw_device);
        case CAP_PROP_HW_ACCELERATION_USE_OPENCL: return static_cast<double>(use_opencl);
#endif  // USE_AV_HW_CODECS
        case CAP_PROP_STREAM_OPEN_TIME_USEC:
            // ic->start_time_realtime is in microseconds
            return ((double)ic->start_time_realtime);
        default: break;
    }

    return 0;
}

double CvCapture_FFMPEG::r2d(AVRational r) const { return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den; }

double CvCapture_FFMPEG::get_duration_sec() const {
    double sec = (double)ic->duration / (double)AV_TIME_BASE;

    if (sec < eps_zero) {
        sec = (double)ic->streams[video_stream]->duration * r2d(ic->streams[video_stream]->time_base);
    }

    return sec;
}

int64_t CvCapture_FFMPEG::get_bitrate() const { return ic->bit_rate / 1000; }

double CvCapture_FFMPEG::get_fps() const {
#if 0 && LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(55, 1, 100) && LIBAVFORMAT_VERSION_MICRO >= 100
	double fps = r2d(av_guess_frame_rate(ic, ic->streams[video_stream], NULL));
#else
    double fps = r2d(ic->streams[video_stream]->avg_frame_rate);

#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(52, 111, 0)
    if (fps < eps_zero) {
        fps = r2d(ic->streams[video_stream]->avg_frame_rate);
    }
#endif

    if (fps < eps_zero) {
        fps = 1.0 / r2d(ic->streams[video_stream]->codec->time_base);
    }
#endif
    return fps;
}

int64_t CvCapture_FFMPEG::get_total_frames() const {
    int64_t nbf = ic->streams[video_stream]->nb_frames;

    if (nbf == 0) {
        nbf = (int64_t)floor(get_duration_sec() * get_fps() + 0.5);
    }
    return nbf;
}

int64_t CvCapture_FFMPEG::dts_to_frame_number(int64_t dts) {
    double sec = dts_to_sec(dts);
    return (int64_t)(get_fps() * sec + 0.5);
}

double CvCapture_FFMPEG::dts_to_sec(int64_t dts) const { return (double)(dts - ic->streams[video_stream]->start_time) * r2d(ic->streams[video_stream]->time_base); }

void CvCapture_FFMPEG::get_rotation_angle() {
    rotation_angle = 0;
#if LIBAVUTIL_BUILD >= CALC_FFMPEG_VERSION(52, 94, 100)
    AVDictionaryEntry* rotate_tag = av_dict_get(video_st->metadata, "rotate", NULL, 0);
    if (rotate_tag != NULL) rotation_angle = atoi(rotate_tag->value);
#endif
}

void CvCapture_FFMPEG::seek(int64_t _frame_number) {
    _frame_number = std::min(_frame_number, get_total_frames());
    int delta = 16;

    // if we have not grabbed a single frame before first seek, let's read the first frame
    // and get some valuable information during the process
    if (first_frame_number < 0 && get_total_frames() > 1) grabFrame();

    for (;;) {
        int64_t _frame_number_temp = std::max(_frame_number - delta, (int64_t)0);
        double sec = (double)_frame_number_temp / get_fps();
        int64_t time_stamp = ic->streams[video_stream]->start_time;
        double time_base = r2d(ic->streams[video_stream]->time_base);
        time_stamp += (int64_t)(sec / time_base + 0.5);
        if (get_total_frames() > 1) av_seek_frame(ic, video_stream, time_stamp, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(ic->streams[video_stream]->codec);
        if (_frame_number > 0) {
            grabFrame();

            if (_frame_number > 1) {
                frame_number = dts_to_frame_number(picture_pts) - first_frame_number;
                // printf("_frame_number = %d, frame_number = %d, delta = %d\n",
                //       (int)_frame_number, (int)frame_number, delta);

                if (frame_number < 0 || frame_number > _frame_number - 1) {
                    if (_frame_number_temp == 0 || delta >= INT_MAX / 4) break;
                    delta = delta < 16 ? delta * 2 : delta * 3 / 2;
                    continue;
                }
                while (frame_number < _frame_number - 1) {
                    if (!grabFrame()) break;
                }
                frame_number++;
                break;
            } else {
                frame_number = 1;
                break;
            }
        } else {
            frame_number = 0;
            break;
        }
    }
}

void CvCapture_FFMPEG::seek(double sec) { seek((int64_t)(sec * get_fps() + 0.5)); }

bool CvCapture_FFMPEG::setProperty(int property_id, double value) {
    if (!video_st) return false;

    switch (property_id) {
        case CAP_PROP_POS_MSEC:
        case CAP_PROP_POS_FRAMES:
        case CAP_PROP_POS_AVI_RATIO: {
            switch (property_id) {
                case CAP_PROP_POS_FRAMES: seek((int64_t)value); break;

                case CAP_PROP_POS_MSEC: seek(value / 1000.0); break;

                case CAP_PROP_POS_AVI_RATIO: seek((int64_t)(value * ic->duration)); break;
            }

            picture_pts = (int64_t)value;
        } break;
        case CAP_PROP_FORMAT:
            if (value == -1) return setRaw();
            return false;
        case CAP_PROP_ORIENTATION_AUTO:
#if LIBAVUTIL_BUILD >= CALC_FFMPEG_VERSION(52, 94, 100)
            rotation_auto = value != 0 ? true : false;
            return true;
#else
            rotation_auto = false;
            return false;
#endif
        default: return false;
    }

    return true;
}

///////////////// FFMPEG CvVideoWriter implementation //////////////////////////

#define CV_PRINTABLE_CHAR(ch) ((ch) < 32 ? '?' : (ch))
#define CV_TAG_TO_PRINTABLE_CHAR4(tag) CV_PRINTABLE_CHAR((tag)&255), CV_PRINTABLE_CHAR(((tag) >> 8) & 255), CV_PRINTABLE_CHAR(((tag) >> 16) & 255), CV_PRINTABLE_CHAR(((tag) >> 24) & 255)

static inline bool cv_ff_codec_tag_match(const AVCodecTag* tags, CV_CODEC_ID id, unsigned int tag) {
    while (tags->id != AV_CODEC_ID_NONE) {
        if (tags->id == id && tags->tag == tag) return true;
        tags++;
    }
    return false;
}

static inline bool cv_ff_codec_tag_list_match(const AVCodecTag* const* tags, CV_CODEC_ID id, unsigned int tag) {
    int i;
    for (i = 0; tags && tags[i]; i++) {
        bool res = cv_ff_codec_tag_match(tags[i], id, tag);
        if (res) return res;
    }
    return false;
}

static inline void cv_ff_codec_tag_dump(const AVCodecTag* const* tags) {
    int i;
    for (i = 0; tags && tags[i]; i++) {
        const AVCodecTag* ptags = tags[i];
        while (ptags->id != AV_CODEC_ID_NONE) {
            unsigned int tag = ptags->tag;
            printf("fourcc tag 0x%08x/'%c%c%c%c' codec_id %04X\n", tag, CV_TAG_TO_PRINTABLE_CHAR4(tag), ptags->id);
            ptags++;
        }
    }
}

static CvCapture_FFMPEG* cvCreateFileCaptureWithParams_FFMPEG(const char* filename, const VideoCaptureParameters& params) {
    // FIXIT: remove unsafe malloc() approach
    CvCapture_FFMPEG* capture = (CvCapture_FFMPEG*)malloc(sizeof(*capture));
    if (!capture) return 0;
    capture->init();
    if (capture->open(filename, params)) return capture;

    capture->close();
    free(capture);
    return 0;
}

void cvReleaseCapture_FFMPEG(CvCapture_FFMPEG** capture) {
    if (capture && *capture) {
        (*capture)->close();
        free(*capture);
        *capture = 0;
    }
}

int cvSetCaptureProperty_FFMPEG(CvCapture_FFMPEG* capture, int prop_id, double value) { return capture->setProperty(prop_id, value); }

double cvGetCaptureProperty_FFMPEG(CvCapture_FFMPEG* capture, int prop_id) { return capture->getProperty(prop_id); }

int cvGrabFrame_FFMPEG(CvCapture_FFMPEG* capture) { return capture->grabFrame(); }

int cvRetrieveFrame_FFMPEG(CvCapture_FFMPEG* capture, unsigned char** data, int* step, int* width, int* height, int* cn, bool rgb) { return capture->retrieveFrame(0, data, step, width, height, cn, rgb); }
