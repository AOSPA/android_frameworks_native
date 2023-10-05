/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Indentifier: BSD-3-Clause-Clear
 */
#include "QtiBufferQueueProducerExtension.h"

#include <dlfcn.h>
#include <log/log.h>
#include <pthread.h>

struct Afp_Interface {
    void *mPenguinHandle = nullptr;
    bool (*mPenguinInit)() = nullptr;
    void (*mPenguinQueueBuffer)(const uint64_t objAddr, const char* name,
                                const bool isAutoTimestamp,
                                const int64_t requestedPresentTimestamp,
                                const int window_api) = nullptr;
    void (*mPenguinRemoveItemFromList)(uint64_t objAddr) = nullptr;
    bool mAllPenguinSymbolsFound = false;
    pthread_once_t mInitControl = PTHREAD_ONCE_INIT;
};

static Afp_Interface AFP;
void AFPLoadAndInit() {
    AFP.mPenguinHandle = dlopen("libpenguin.so", RTLD_NOW);
    if (!AFP.mPenguinHandle) {
        ALOGE("Unable to open libpenguin.so: %s.", dlerror());
    } else {
        AFP.mPenguinInit=
            (bool (*) ())dlsym(AFP.mPenguinHandle, "penguinInit");
        AFP.mPenguinQueueBuffer =
            (void (*) (const uint64_t, const char*, const bool,
                       const int64_t, const int))dlsym(AFP.mPenguinHandle, "penguinQueueBuffer");
        AFP.mPenguinRemoveItemFromList =
            (void (*) (uint64_t))dlsym(AFP.mPenguinHandle, "penguinRemoveItemFromList");
        AFP.mAllPenguinSymbolsFound = AFP.mPenguinInit && AFP.mPenguinQueueBuffer
                                                       && AFP.mPenguinRemoveItemFromList;
        if (!AFP.mAllPenguinSymbolsFound || !AFP.mPenguinInit()) {
            AFP.mAllPenguinSymbolsFound = false;
            dlclose(AFP.mPenguinHandle);
        }
    }
}

namespace android::libguiextension {

QtiBufferQueueProducerExtension::QtiBufferQueueProducerExtension(BufferQueueProducer* bufferQueueProducer)
      : mQtiBufferQueueProducer(bufferQueueProducer) {
    if (!mQtiBufferQueueProducer) {
        ALOGW("Invalid pointer to BufferQueueProducer passed");
    } else {
        ALOGV("Successfully created QtiBufferQueueProducerExtension");
        pthread_once(&(AFP.mInitControl), AFPLoadAndInit);
    }
}

QtiBufferQueueProducerExtension::~QtiBufferQueueProducerExtension() {
   if (AFP.mAllPenguinSymbolsFound) {
       AFP.mPenguinRemoveItemFromList((uint64_t)(this));
   }
}

void QtiBufferQueueProducerExtension::qtiQueueBuffer(bool isAutoTimestamp,
        int64_t requestedPresentTimestamp, int window_api) {
   if (mQtiBufferQueueProducer && AFP.mAllPenguinSymbolsFound) {
       AFP.mPenguinQueueBuffer((uint64_t)this, mQtiBufferQueueProducer->mConsumerName.c_str(),
                               isAutoTimestamp, requestedPresentTimestamp, window_api);
   }
}

} //namespace android::libguiextension
