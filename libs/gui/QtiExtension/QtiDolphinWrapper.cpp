
/* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <dlfcn.h>
#include <thread>
#include <mutex>

#include <log/log.h>

#include "QtiDolphinWrapper.h"


namespace android {

static bool sIsGame = false;

QtiDolphinWrapper::QtiDolphinWrapper() {
    sIsGame = true;
    mQtiDolphinHandle = dlopen("libdolphin.so", RTLD_NOW);
    if (!mQtiDolphinHandle) {
        ALOGW("Unable to open libdolphin.so: %s.", dlerror());
    } else {
        qtiDolphinAppInit = (void (*) ())dlsym(mQtiDolphinHandle, "aDolphinAppInit");
        qtiDolphinSmartTouchActive = (bool (*) ())dlsym(mQtiDolphinHandle,
                "aDolphinSmartTouchActive");
        qtiDolphinQueueBuffer = (void (*) (bool))dlsym(mQtiDolphinHandle, "aDolphinQueueBuffer");
        qtiDolphinFilterBuffer = (void (*) (bool&, nsecs_t&, uint32_t&))dlsym(mQtiDolphinHandle,
                 "aDolphinFilterBuffer");
        bool functionsFound = qtiDolphinAppInit && qtiDolphinSmartTouchActive &&
                              qtiDolphinQueueBuffer && qtiDolphinFilterBuffer;
        if (functionsFound) {
            qtiDolphinAppInit();
        } else {
            ALOGW("Unable to find dolphin functions!");
            dlclose(mQtiDolphinHandle);
            qtiDolphinAppInit = nullptr;
            qtiDolphinSmartTouchActive = nullptr;
            qtiDolphinQueueBuffer = nullptr;
            qtiDolphinFilterBuffer = nullptr;
        }
    }
}

QtiDolphinWrapper::~QtiDolphinWrapper() {
    if (mQtiDolphinHandle) {
        dlclose(mQtiDolphinHandle);
    }
}

QtiDolphinWrapper* QtiDolphinWrapper::sInstance = nullptr;
static std::mutex sInstanceMutex;

QtiDolphinWrapper* QtiDolphinWrapper::qtiGetDolphinWrapper() {
    if (sIsGame) {
        return qtiGetInstanceForGame();
    }
    // return nullptr for non-game to avoid library loading.
    return nullptr;
}

QtiDolphinWrapper* QtiDolphinWrapper::qtiGetInstanceForGame() {
    if (sInstance == nullptr) {
        std::lock_guard<std::mutex> lock(sInstanceMutex);
        // additional check for concurrency
        if (sInstance == nullptr) {
            sInstance = new QtiDolphinWrapper();
        }
    }
    return sInstance;
}
} // namespace android
