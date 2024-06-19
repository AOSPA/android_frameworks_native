
/* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <dlfcn.h>
#include <thread>
#include <mutex>

#include <log/log.h>

#include "QtiInputDolphinWrapper.h"


namespace android {

QtiInputDolphinWrapper::QtiInputDolphinWrapper() {
    mQtiDolphinHandle = dlopen("libdolphin.so", RTLD_NOW);
    if (!mQtiDolphinHandle) {
        ALOGW("Unable to open libdolphin.so: %s.", dlerror());
    } else {
        qtiDolphinSetTouchEvent = (void (*) (int))dlsym(mQtiDolphinHandle,
                "aDolphinSetTouchEvent");
        qtiDolphinConsumeInputNow = (void (*) (bool&))dlsym(mQtiDolphinHandle,
                "aDolphinConsumeInputNow");
        bool functionsFound = qtiDolphinSetTouchEvent && qtiDolphinConsumeInputNow;
        if (!functionsFound) {
            ALOGW("Unable to find dolphin functions!");
            dlclose(mQtiDolphinHandle);
            qtiDolphinSetTouchEvent = nullptr;
            qtiDolphinConsumeInputNow = nullptr;
        }
    }
}

QtiInputDolphinWrapper::~QtiInputDolphinWrapper() {
    if (mQtiDolphinHandle) {
        dlclose(mQtiDolphinHandle);
    }
}

QtiInputDolphinWrapper* QtiInputDolphinWrapper::sInstance = nullptr;
static std::mutex sInstanceMutex;

QtiInputDolphinWrapper* QtiInputDolphinWrapper::qtiGetDolphinWrapper() {
    // sInstance may be nullprt unless qtiGetInstance() is called.
    return sInstance;
}

QtiInputDolphinWrapper* QtiInputDolphinWrapper::qtiGetInstance() {
    if (sInstance == nullptr) {
        std::lock_guard<std::mutex> lock(sInstanceMutex);
        // additional check for concurrency
        if (sInstance == nullptr) {
            sInstance = new QtiInputDolphinWrapper();
        }
    }
    return sInstance;
}

} // namespace android
