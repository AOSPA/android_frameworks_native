/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiBLASTBufferQueueExtension.h"

#include <log/log.h>

namespace android::libguiextension {

QtiBLASTBufferQueueExtension::QtiBLASTBufferQueueExtension(BLASTBufferQueue* blastBufferQueue)
      : mQtiBlastBufferQueue(blastBufferQueue) {
    if (!mQtiBlastBufferQueue) {
        ALOGW("Invalid pointer to BLASTBufferQueue passed");
    } else {
        ALOGV("Successfully created QtiBLASTBufferQueueExtension");
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

} // namespace android::libguiextension
