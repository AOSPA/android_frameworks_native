/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiFramebufferSurfaceExtension.h"

#include <log/log.h>

namespace android::surfaceflingerextension {

QtiFramebufferSurfaceExtension::QtiFramebufferSurfaceExtension(LegacyFramebufferSurface* fbs)
      : mQtiFBSLegacy(fbs) {
    if (!mQtiFBSLegacy) {
        ALOGW("Passed an invalid pointer to FramebufferSurface");
    }

    ALOGV("Successfully created QtiFBSExtension %p", mQtiFBS);
    using_legacy_fbs_ = true;
}

QtiFramebufferSurfaceExtension::QtiFramebufferSurfaceExtension(FramebufferSurface* fbs)
      : mQtiFBS(fbs) {
    if (!mQtiFBS) {
        ALOGW("Passed an invalid pointer to FramebufferSurface");
    }

    ALOGV("Successfully created QtiFBSExtension %p", mQtiFBS);
    using_legacy_fbs_ = false;
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
    if (using_legacy_fbs_) {
        return mQtiFBSLegacy->mCurrentBufferSlot;
    } else {
        // if no buffer, return INVALID_BUFFER_SLOT (-1)
        if (!mQtiFBS->mFrameData->mBuffer) {
            return -1;
        }
        Mutex::Autolock lock(mQtiFBS->mMutex);
        auto slot = mQtiFBS->mHwcSlotTracker.getSlot(mQtiFBS->mFrameData->mBuffer);
        return static_cast<int>(slot.slot);
    }
}

ui::Dataspace QtiFramebufferSurfaceExtension::getClientTargetCurrentDataspace() {
    if (using_legacy_fbs_) {
        return mQtiFBSLegacy->mDataspace;
    } else {
        return mQtiFBS->mDataspace;
    }
}

} // namespace android::surfaceflingerextension
