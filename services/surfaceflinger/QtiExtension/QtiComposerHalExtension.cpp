/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "QtiComposerHalExtension.h"

#include <android-base/properties.h>

namespace android::surfaceflingerextension {

QtiComposerHalExtension::QtiComposerHalExtension() {}

QtiComposerHalExtension::QtiComposerHalExtension(Hwc2::Composer* composerHal) {
    if (!composerHal) {
        ALOGW("Passed an invalid pointer to composer hal");
        return;
    }

    mQtiHidlComposerExtn =
            new QtiHidlComposerHalExtension(static_cast<Hwc2::HidlComposer*>(composerHal));
    mQtiComposerHal = static_cast<Hwc2::HidlComposer*>(composerHal);
}

Error QtiComposerHalExtension::qtiSetDisplayElapseTime(Display display, uint64_t timeStamp) {
    return mQtiHidlComposerExtn->qtiSetDisplayElapseTime(display, timeStamp);
}

} // namespace android::surfaceflingerextension
