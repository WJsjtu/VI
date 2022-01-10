#pragma once

typedef struct CvCapture_FFMPEG CvCapture_FFMPEG;

//OPENCV_FFMPEG_API struct CvCapture_FFMPEG* cvCreateFileCapture_FFMPEG(const char* filename);
int cvSetCaptureProperty_FFMPEG(struct CvCapture_FFMPEG* cap,
	int prop, double value);
double cvGetCaptureProperty_FFMPEG(struct CvCapture_FFMPEG* cap, int prop);
int cvGrabFrame_FFMPEG(struct CvCapture_FFMPEG* cap);
int cvRetrieveFrame_FFMPEG(struct CvCapture_FFMPEG* capture, unsigned char** data,
	int* step, int* width, int* height, int* cn);
void cvReleaseCapture_FFMPEG(struct CvCapture_FFMPEG** cap);