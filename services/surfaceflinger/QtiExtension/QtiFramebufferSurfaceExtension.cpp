/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiFramebufferSurfaceExtension.h"

#include <log/log.h>

namespace android::surfaceflingerextension {

QtiFramebufferSurfaceExtension::QtiFramebufferSurfaceExtension(FramebufferSurface* fbs)
      : mQtiFBS(fbs) {
    if (!mQtiFBS) {
        ALOGW("Passed an invalid pointer to FramebufferSurface");
    }

    ALOGV("Successfully created QtiFBSExtension %p", mQtiFBS);
}

uint64_t QtiFramebufferSurfaceExtension::qtiSetOutputUsage() {
    ALOGW("%s should not be called from QtiFramebufferSurfaceExtension", __func__);
    return 0;
}

uint64_t QtiFramebufferSurfaceExtension::qtiSetOutputUsage(uint64_t flag) {
    ALOGW("%s should not be called from QtiFramebufferSurfaceExtension", __func__);
    return 0;
}

uint64_t QtiFramebufferSurfaceExtension::qtiExcludeVideoFromScratchBuffer(std::string source,
                                                                          uint64_t usage) {
    ALOGW("%s should not be called from QtiFramebufferSurfaceExtension", __func__);
    return 0;
}

int QtiFramebufferSurfaceExtension::getClientTargetCurrentSlot() {
    return mQtiFBS->mCurrentBufferSlot;
}

ui::Dataspace QtiFramebufferSurfaceExtension::getClientTargetCurrentDataspace() {
    return mQtiFBS->mDataspace;
}

} // namespace android::surfaceflingerextension
