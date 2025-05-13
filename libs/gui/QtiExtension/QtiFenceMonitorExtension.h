/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <utils/Log.h>
#include <utils/Trace.h>
#include <utils/Timers.h>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace android {

namespace gui {
class FenceMonitor;
}

namespace libguiextension {

class QtiFenceMonitorExtension {
public:
    static bool qtiGetGPUBigJankEnabled();
    static QtiFenceMonitorExtension* qtiGetInstance(gui::FenceMonitor* fenceMonitor);
    void qtiQueueFence(bool start);

private:
    QtiFenceMonitorExtension();
    ~QtiFenceMonitorExtension() = default;
    static QtiFenceMonitorExtension* sInstance;
    std::thread mMonitorThread;
    std::condition_variable mCondition;
    std::mutex mMutex;
    bool mMonitoringStart = false;
    bool mMonitoringStop = false;
    bool mTimeout = false;
    void monitor();
    static void qtiPropertyMonitor(const std::string& prop);
    void qtiSendGPUJank();
    static bool isTopApp();
};

} // namespace libguiextension
} // namespace android
