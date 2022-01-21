#pragma once

#if defined __linux__ || defined __APPLE__ || defined __HAIKU__
#include <unistd.h>  // -D_FORTIFY_SOURCE=2 workaround: https://github.com/opencv/opencv/issues/15020
#endif

#ifdef NDEBUG
#define CV_LOG_STRIP_LEVEL CV_LOG_LEVEL_DEBUG + 1
#else
#define CV_LOG_STRIP_LEVEL CV_LOG_LEVEL_VERBOSE + 1
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>  // FIXIT remove this

#if defined _WIN32 || defined WINCE
#if !defined _WIN32_WINNT
#ifdef HAVE_MSMF
#define _WIN32_WINNT 0x0600  // Windows Vista
#else
#define _WIN32_WINNT 0x0501  // Windows XP
#endif
#endif

#include <windows.h>
#undef small
#undef min
#undef max
#undef abs
#endif

#include "cap_interface.hpp"
