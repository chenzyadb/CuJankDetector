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

#define ATRACE_TAG_GRAPHICS (1<<1)
#define MSG_PATH "/dev/jank.message"

static uint64_t GetTimeStampMs() 
{
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void MsgInit()
{
    int fd = open(MSG_PATH, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd >= 0) {
        char buffer[128] = { 0 };
        sprintf(buffer, "[INIT] %" PRIu64 "\n", GetTimeStampMs());
        write(fd, buffer, sizeof(buffer));
        close(fd);
    } else {
        ALOGE("Failed to open message path.");
    }
}

static void MsgError()
{
    int fd = open(MSG_PATH, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd >= 0) {
        char buffer[128] = { 0 };
        sprintf(buffer, "[ERROR] %" PRIu64 "\n", GetTimeStampMs());
        write(fd, buffer, sizeof(buffer));
        close(fd);
    } else {
        ALOGE("Failed to open message path.");
    }
}

static void MsgJank()
{
    int fd = open(MSG_PATH, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd >= 0) {
        char buffer[128] = { 0 };
        sprintf(buffer, "[JANK] %" PRIu64 "\n", GetTimeStampMs());
        write(fd, buffer, sizeof(buffer));
        close(fd);
    } else {
        ALOGE("Failed to open message path.");
    }
}

static void MsgBigJank()
{
    int fd = open(MSG_PATH, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd >= 0) {
        char buffer[128] = { 0 };
        sprintf(buffer, "[BIG_JANK] %" PRIu64 "\n", GetTimeStampMs());
        write(fd, buffer, sizeof(buffer));
        close(fd);
    } else {
        ALOGE("Failed to open message path.");
    }
}

static void DetectJank()
{
    static int frameTimeHistory[3] = {40, 40, 40};
    static uint64_t lastCompositeTime = GetTimeStampMs();
    int curFrameTime = 0;
    {
        auto compositeTime = GetTimeStampMs();
        curFrameTime = (int)(compositeTime - lastCompositeTime);
        lastCompositeTime = compositeTime;
    }
    if (curFrameTime < 1000 / 5 && curFrameTime > 1000 / 120) {
        int avgFrameTime = (frameTimeHistory[0] + frameTimeHistory[1] + frameTimeHistory[2]) / 3;
        if (curFrameTime > avgFrameTime * 2) {
            if (curFrameTime > 1000 / 24) {
                MsgBigJank();
                ALOGV("Detected Big Jank, FrameTime=%d ms.", curFrameTime);
            } else {
                MsgJank();
                ALOGV("Detected Jank, FrameTime=%d ms.", curFrameTime);
            }
        }
        if (curFrameTime < 1000 / 24) {
            frameTimeHistory[0] = frameTimeHistory[1];
            frameTimeHistory[1] = frameTimeHistory[2];
            frameTimeHistory[2] = curFrameTime;
        }
    }
}

static uint64_t (*orig_atrace_get_enabled_tags)();
static void (*orig_atrace_begin_body)(const char*);

extern "C" {
    __attribute__((visibility("default")))
    uint64_t atrace_get_enabled_tags_() 
    {
        auto enabled_tags = (*orig_atrace_get_enabled_tags)();
        enabled_tags |= ATRACE_TAG_GRAPHICS; 

        return enabled_tags;
    }

    __attribute__((visibility("default")))
    void atrace_begin_body_(const char* name)
    {
        if ((*orig_atrace_get_enabled_tags)() & ATRACE_TAG_GRAPHICS) {
            (*orig_atrace_begin_body)(name);
        }
        if (strstr(name, "postComposition")) {
            DetectJank();
        }
    }
}

static void HookSurfaceflinger() 
{
    auto atrace_get_enabled_tags_addr = DobbySymbolResolver(nullptr, "atrace_get_enabled_tags");
    auto atrace_begin_body_addr = DobbySymbolResolver(nullptr, "atrace_begin_body");
    if (atrace_get_enabled_tags_addr && atrace_begin_body_addr) {
        DobbyHook(atrace_get_enabled_tags_addr, (void*)&atrace_get_enabled_tags_, (void**)&orig_atrace_get_enabled_tags);
        DobbyHook(atrace_begin_body_addr, (void*)&atrace_begin_body_, (void**)&orig_atrace_begin_body);
        ALOGI("Successfully hook surfaceflinger.");
    } else {
        MsgError();
        ALOGE("Failed to hook surfaceflinger.");
    }
}

extern "C" {
    __attribute__((visibility("default")))
    __attribute__((constructor))
    void start_hook(void)
    {
        MsgInit();
        HookSurfaceflinger();
    }
}
