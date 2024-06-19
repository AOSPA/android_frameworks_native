
/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <dlfcn.h>

#include <log/log.h>

#include "QtiDolphinWrapper.h"


namespace android::surfaceflingerextension {

QtiDolphinWrapper::QtiDolphinWrapper() {
    mQtiDolphinHandle = dlopen("libdolphin.so", RTLD_NOW);
    if (!mQtiDolphinHandle) {
        ALOGW("Unable to open libdolphin.so: %s.", dlerror());
    } else {
        qtiDolphinInit = (bool (*) ())dlsym(mQtiDolphinHandle, "dolphinInit");
        qtiDolphinSetVsyncPeriod = (void (*) (nsecs_t)) dlsym(mQtiDolphinHandle,
                "dolphinSetVsyncPeriod");
        qtiDolphinTrackBufferIncrement = (void (*) (const char*, bool, nsecs_t))dlsym(
                 mQtiDolphinHandle, "dolphinTrackBufferIncrement");
        qtiDolphinTrackBufferDecrement = (void (*) (const char*, int))dlsym(mQtiDolphinHandle,
                "dolphinTrackBufferDecrement");
        qtiDolphinTrackVsyncSignal = (void (*) ())dlsym(mQtiDolphinHandle,
                "dolphinTrackVsyncSignal");
        qtiDolphinUnblockPendingBuffer = (void (*) ())dlsym(mQtiDolphinHandle,
                "dolphinUnblockPendingBuffer");
        bool functionsFound = qtiDolphinInit && qtiDolphinSetVsyncPeriod &&
                              qtiDolphinTrackBufferIncrement && qtiDolphinTrackBufferDecrement &&
                              qtiDolphinTrackVsyncSignal && qtiDolphinUnblockPendingBuffer;
        if (functionsFound) {
            qtiDolphinInit();
        } else {
            ALOGW("Unable to find dolphin functions!");
            dlclose(mQtiDolphinHandle);
            qtiDolphinInit = nullptr;
            qtiDolphinSetVsyncPeriod = nullptr;
            qtiDolphinTrackBufferIncrement = nullptr;
            qtiDolphinTrackBufferDecrement = nullptr;
            qtiDolphinTrackVsyncSignal = nullptr;
            qtiDolphinUnblockPendingBuffer = nullptr;
        }
    }
}

QtiDolphinWrapper::~QtiDolphinWrapper() {
    if (mQtiDolphinHandle) {
        dlclose(mQtiDolphinHandle);
    }
}

} // namespace android::surfaceflingerextension
