/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#define LOG_NDEBUG 0
#include "QtiHidlComposerHalExtension.h"

namespace android::surfaceflingerextension {

QtiHidlComposerHalExtension::QtiHidlComposerHalExtension(Hwc2::Composer* composerHal) {
    if (!composerHal) {
        ALOGW("Passed an invalid pointer to composer hal");
        return;
    }

    mQtiHidlComposer = static_cast<Hwc2::HidlComposer*>(composerHal);
    ALOGV("Successfully created QtiHidlComposerHalExtension %p", mQtiHidlComposer);
}

Error QtiHidlComposerHalExtension::qtiSetDisplayElapseTime(Display display, uint64_t timeStamp) {
    mQtiHidlComposer->mWriter.selectDisplay(display);
    mQtiHidlComposer->mWriter.qtiSetDisplayElapseTime(timeStamp);
    return Error::NONE;
}

} // namespace android::surfaceflingerextension
