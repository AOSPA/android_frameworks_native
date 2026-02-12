/* Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiBLASTBufferQueueExtension.h"
#include "QtiDolphinWrapper.h"
#include "QtiSurfaceUtils.h"

#include <pthread.h>
#include <regex>
#include <binder/IServiceManager.h>
#include <log/log.h>

namespace android::libguiextension {

static bool sQtiIsGame = false;
static QtiDolphinWrapper* sQtiDolphinWrapper = nullptr;
static bool sQtiSmartTouchActive = false;
static std::string sQtiLayerName = "";
static pthread_once_t sQtiCheckAppTypeOnce = PTHREAD_ONCE_INIT;
static void qtiInitAppType() {
    sp<IBinder> sPerfService = getPerfService();
    if (sPerfService == nullptr) {
        ALOGE("Cannot find perfservice");
        return;
    }
    String16 ifName = sPerfService->getInterfaceDescriptor();
    if (ifName.size() > 0) {
        const std::regex re("(?:\\w*\\s*SurfaceView\\[)([^/]+).*");
        std::smatch match;
        if (!std::regex_match(sQtiLayerName, match, re)) {
            return;
        }
        String16 pkgName = String16(match[1].str().c_str());

        Parcel data, reply;
        int GAME_TYPE = 2;
        int VENDOR_FEEDBACK_WORKLOAD_TYPE = 0x00001601;
        int PERF_GET_FEEDBACK = IBinder::FIRST_CALL_TRANSACTION + 7;

        // Query flags: QUERY_AppType | QUERY_QUICK_TOUCH (1<<16)
        int queryFlags = 0x1FFFF;
        // Perfservice uses the first parameter of the array as the length of the list passed into perfhal.
        int array[3] = {2, 0, queryFlags};
        data.markForBinder(sPerfService);
        data.writeInterfaceToken(ifName);
        data.writeInt32(VENDOR_FEEDBACK_WORKLOAD_TYPE);
        data.writeString16(pkgName);
        data.writeInt32(getpid());
        data.writeInt32Array(3, array);
        sPerfService->transact(PERF_GET_FEEDBACK, data, &reply);
        reply.readExceptionCode();

        // Parse combined result, app type (low 16 bits), Quick Touch status (bit 16)
        // set FEATURE_QUICK_TOUCH(1 << 16) bit 1 when quick touch is disabled.
        int combinedResult = reply.readInt32();
        int appType = combinedResult & 0xFFFF;
        bool isQuickTouchDisabled = (combinedResult & (1 << 16)) != 0;
        if (appType == GAME_TYPE) {
            sQtiIsGame = true;
            if (isQuickTouchDisabled) {
                return;
            }

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

} // namespace android::libguiextension
