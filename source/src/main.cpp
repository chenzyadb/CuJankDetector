#define _GNU_SOURCE 1
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <android/log.h>
#include "utils/dobby.h"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "CU_JANK_DETECTOR", __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "CU_JANK_DETECTOR", __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, "CU_JANK_DETECTOR", __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, "CU_JANK_DETECTOR", __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "CU_JANK_DETECTOR", __VA_ARGS__)
#define ALOGF(...) __android_log_print(ANDROID_LOG_FATAL, "CU_JANK_DETECTOR", __VA_ARGS__)

static uint64_t GetTimeStampMs() 
{
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void MsgInit();
static void MsgError();
static void MsgJank();
static void MsgBigJank();
static void DetectJank();
static void HookSurfaceflinger();

extern "C" {
    __attribute__((visibility("default")))
    void start_hook(void)
    {
        MsgInit();
        HookSurfaceflinger();
    }
}
