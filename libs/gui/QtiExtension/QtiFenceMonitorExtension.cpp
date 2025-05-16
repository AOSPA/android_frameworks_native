/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <atomic>
#include <cutils/properties.h>
#include <android-base/properties.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <chrono>
#include <string.h>
#include <fstream>

#include "QtiFenceMonitorExtension.h"
#include "../include/gui/FenceMonitor.h"

#define BIG_JANK_DETECT_ENABLED "debug.perf.enable_big_jank_detect"
#define BIG_JANK_THRESHOLD_MS "debug.perf.gpu_big_jank_threshold_ms"

namespace android {

namespace libguiextension {

static sp<IBinder> sPerfService = nullptr;
static bool sBigJankEnabled = property_get_bool(BIG_JANK_DETECT_ENABLED, false);
static bool sCheckDone = false;
static bool sIsTopApp = false;
static std::atomic<int32_t> sThresholdMs(0);

QtiFenceMonitorExtension::QtiFenceMonitorExtension() {
    sp<IServiceManager> sm = defaultServiceManager();
    sPerfService = sm->checkService(String16("vendor.perfservice"));
    if (sPerfService == nullptr) {
        ALOGE("Cannot find perfservice");
        return;
    }
    qtiPropertyMonitor(BIG_JANK_THRESHOLD_MS);
    mMonitorThread = std::thread(&QtiFenceMonitorExtension::monitor, this);
    pthread_setname_np(mMonitorThread.native_handle(), "GPUMonitor");
    mMonitorThread.detach();
}

bool QtiFenceMonitorExtension::isTopApp() {
    std::ifstream file("/proc/self/cgroup");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("top-app") != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool QtiFenceMonitorExtension::qtiGetGPUBigJankEnabled() {
    if (!sBigJankEnabled) {
        return false;
    }
    if (!sCheckDone) {
        sIsTopApp = isTopApp();
        sCheckDone = true;
    }
    return sIsTopApp;
}

QtiFenceMonitorExtension* QtiFenceMonitorExtension::sInstance = nullptr;
static std::mutex sInstanceMutex;

QtiFenceMonitorExtension* QtiFenceMonitorExtension::qtiGetInstance(gui::FenceMonitor* fenceMonitor) {
    if (!sBigJankEnabled) {
        return nullptr;
    }
    if (sInstance == nullptr) {
        if (!fenceMonitor) {
            ALOGW("Invalid pointer to FenceMonitor passed");
            return nullptr;
        }
        std::lock_guard<std::mutex> lock(sInstanceMutex);
        if (sInstance == nullptr) {
            sInstance = new QtiFenceMonitorExtension();
        }
    }
    return sInstance;
}

void QtiFenceMonitorExtension::qtiPropertyMonitor(const std::string& prop){
    std::thread propertyMonitorThread([prop]() {
        const std::string property = prop;
        base::CachedProperty cached_property(property);
        while (true) {
            cached_property.WaitForChange();
            int32_t value = property_get_int32(property.c_str(), 0);
            ALOGI("Prop Changed: %s = %d", prop.c_str(), value);
            if (property == BIG_JANK_THRESHOLD_MS) {
                sThresholdMs.store(value);
                if (sThresholdMs.load() <= 0) {
                    ALOGE("Invalid thresholdMs of gpu_big_jank, disabled");
                } else {
                    ALOGI("The thresholdMs of gpu_big_jank is %dms", sThresholdMs.load());
                }
            }
        }
    });
    propertyMonitorThread.detach();
}

void QtiFenceMonitorExtension::qtiQueueFence(bool start) {
    if (sThresholdMs.load() <= 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(mMutex);
    if (start) {
        mMonitoringStart = true;
        mMonitoringStop = false;
    } else {
        mMonitoringStop = true;
    }
    mCondition.notify_one();
}

void QtiFenceMonitorExtension::monitor() {
    while (true) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondition.wait(lock, [&] { return mMonitoringStart; });
        mMonitoringStart = false;
        ATRACE_NAME("GPU monitoring");
        if (mCondition.wait_for(lock, std::chrono::milliseconds(sThresholdMs.load()),
                [&] { return mMonitoringStop || mMonitoringStart; })) {
            mTimeout = false;
        } else {
            mTimeout = true;
            ALOGI("Send GPU Big jank.");
            qtiSendGPUJank();
        }
    }
}

void QtiFenceMonitorExtension::qtiSendGPUJank() {
    ATRACE_CALL();
    if (sPerfService == nullptr) return;
    String16 ifName = sPerfService->getInterfaceDescriptor();
    int duration = sThresholdMs.load();
    if (ifName.size() > 0) {
        int PERF_HINT = IBinder::FIRST_CALL_TRANSACTION + 2;
        int VENDOR_HINT_GAME_FRAME_RESCUE = 0x0000105A;
        int HINT_TYPE_FOR_GPU_BIG_JANK_DETECT = 4;
        Parcel data, reply;
        data.markForBinder(sPerfService);
        data.writeInterfaceToken(ifName);
        data.writeInt32(VENDOR_HINT_GAME_FRAME_RESCUE);
        data.writeString16(String16(""));
        data.writeInt32(duration);
        data.writeInt32(HINT_TYPE_FOR_GPU_BIG_JANK_DETECT);
        data.writeInt32(-1);
        sPerfService->transact(PERF_HINT, data, &reply);
        reply.readExceptionCode();
    }
}
} // namespace libguiextension
} // namespace android