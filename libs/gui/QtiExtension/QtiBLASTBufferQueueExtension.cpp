/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiBLASTBufferQueueExtension.h"
#include "QtiDolphinWrapper.h"

#include <pthread.h>
#include <regex>
#include <binder/IServiceManager.h>
#include <log/log.h>

#define TID_CHECK_WINDOW 200
#define SWITCH_CHECK_TIME ms2ns(1000)

namespace android::libguiextension {

static sp<IBinder> sPerfService = nullptr;
static bool sQtiIsGame = false;
static pid_t sGameGfxTid = -1;
static int64_t sTimestamp = 0;
static QtiDolphinWrapper* sQtiDolphinWrapper = nullptr;
static bool sQtiSmartTouchActive = false;
static std::string sQtiLayerName = "";
static pthread_once_t sQtiCheckAppTypeOnce = PTHREAD_ONCE_INIT;
static void qtiInitAppType() {
    sp<IServiceManager> sm = defaultServiceManager();
    sPerfService = sm->checkService(String16("vendor.perfservice"));
    if (sPerfService == nullptr) {
        ALOGE("Cannot find perfservice");
        return;
    }
    String16 ifName = sPerfService->getInterfaceDescriptor();
    if (ifName.size() > 0) {
        const std::regex re("(?:SurfaceView\\[)([^/]+).*");
        std::smatch match;
        if (!std::regex_match(sQtiLayerName, match, re)) {
            return;
        }
        String16 pkgName = String16(match[1].str().c_str());

        Parcel data, reply;
        int GAME_TYPE = 2;
        int VENDOR_FEEDBACK_WORKLOAD_TYPE = 0x00001601;
        int PERF_GET_FEEDBACK = IBinder::FIRST_CALL_TRANSACTION + 7;
        int array[0];
        data.markForBinder(sPerfService);
        data.writeInterfaceToken(ifName);
        data.writeInt32(VENDOR_FEEDBACK_WORKLOAD_TYPE);
        data.writeString16(pkgName);
        data.writeInt32(getpid());
        data.writeInt32Array(0, array);
        sPerfService->transact(PERF_GET_FEEDBACK, data, &reply);
        reply.readExceptionCode();
        int type = reply.readInt32();
        if (type == GAME_TYPE) {
            sQtiIsGame = true;

            sQtiDolphinWrapper = QtiDolphinWrapper::qtiGetInstanceForGame();
            if (sQtiDolphinWrapper && sQtiDolphinWrapper->qtiDolphinSmartTouchActive) {
                sQtiSmartTouchActive = sQtiDolphinWrapper->qtiDolphinSmartTouchActive();
            }
        }
    }
}

QtiBLASTBufferQueueExtension::QtiBLASTBufferQueueExtension(BLASTBufferQueue* blastBufferQueue,
        const std::string& name)
      : mQtiBlastBufferQueue(blastBufferQueue) {
    if (!mQtiBlastBufferQueue) {
        ALOGW("Invalid pointer to BLASTBufferQueue passed");
    } else {
        ALOGV("Successfully created QtiBLASTBufferQueueExtension");
    }

    if (name.find("SurfaceView") != std::string::npos) {
        sQtiLayerName = name;
        pthread_once(&sQtiCheckAppTypeOnce, qtiInitAppType);
    }
}

void QtiBLASTBufferQueueExtension::qtiSetConsumerUsageBitsForRC(std::string name,
                                                                sp<SurfaceControl> sc) {
    if (!mQtiBlastBufferQueue) {
        ALOGW("Pointer to BLASTBufferQueue is invalid");
        return;
    }

    uint64_t usage = 0;
    if (strstr(name.c_str(), "ScreenDecorOverlay") != nullptr && sc != nullptr) {
        sp<SurfaceComposerClient> client = sc->getClient();
        if (client != nullptr) {
            // Deprecated getInternalDisplayToken()
            // Since libgui is a vndk, it is not permitted to read vendor properties and cannot be
            // linked to IDisplayConfig. Set the consumer usage bits for RC and let vendor handle
            // this when RC is in disabled state.

            // retain original flags and append SW Flags
            usage = GraphicBuffer::USAGE_HW_COMPOSER | GraphicBuffer::USAGE_HW_TEXTURE |
                    GraphicBuffer::USAGE_SW_READ_RARELY | GraphicBuffer::USAGE_SW_WRITE_RARELY;
            mQtiBlastBufferQueue->mConsumer->setConsumerUsageBits(usage);
            ALOGV("For layer %s, set consumer usage bits to %" PRIu64, name.c_str(), usage);
        }
    } else {
        ALOGV("Avoid setting consumer usage bits to layer %s", name.c_str());
    }
}

bool QtiBLASTBufferQueueExtension::qtiIsGame() {
    return sQtiIsGame;
}

void QtiBLASTBufferQueueExtension::qtiSendGfxTid() {
    if (sPerfService == nullptr) return;
    String16 ifName = sPerfService->getInterfaceDescriptor();
    if (ifName.size() > 0) {
        int PERF_HINT = IBinder::FIRST_CALL_TRANSACTION + 2;
        int VENDOR_HINT_PASS_PID = 0x0000109C;
        int HINT_TYPE_FOR_GAME_GFX_TID = 6;
        Parcel data, reply;
        data.markForBinder(sPerfService);
        data.writeInterfaceToken(ifName);
        data.writeInt32(VENDOR_HINT_PASS_PID);
        data.writeString16(String16(""));
        data.writeInt32(sGameGfxTid);
        data.writeInt32(HINT_TYPE_FOR_GAME_GFX_TID);
        data.writeInt32(sGameGfxTid);
        sPerfService->transact(PERF_HINT, data, &reply);
        reply.readExceptionCode();
    }
}

void QtiBLASTBufferQueueExtension::qtiTrackTransaction(uint64_t frameNumber, int64_t timestamp) {
    if (!sQtiIsGame) return;
    if (sGameGfxTid < 0 || (timestamp - sTimestamp) > SWITCH_CHECK_TIME) {
        // send tid when games launch or resume
        sGameGfxTid = gettid();
        qtiSendGfxTid();
    } else if (frameNumber % TID_CHECK_WINDOW == 0) {
        // sned tid if game gfx tid changes
        pid_t gfxTid = gettid();
        if (gfxTid != sGameGfxTid) {
            sGameGfxTid = gfxTid;
            qtiSendGfxTid();
        }
    }
    sTimestamp = timestamp;
}

} // namespace android::libguiextension
