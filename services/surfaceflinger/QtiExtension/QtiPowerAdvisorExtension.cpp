/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiPowerAdvisorExtension.h"

namespace android::surfaceflingerextension {

QtiPowerAdvisorExtension::QtiPowerAdvisorExtension(Hwc2::impl::PowerAdvisor* powerAdvisor)
      : mQtiPowerAdvisor(powerAdvisor) {
    if (!mQtiPowerAdvisor) {
        ALOGW("Invalid pointer to PowerAdvisor passed");
    } else {
        ALOGV("Successfully created QtiPowerAdvisorExtn %p", mQtiPowerAdvisor);
    }
}

bool QtiPowerAdvisorExtension::qtiCanNotifyDisplayUpdateImminent() {
    if (!mQtiPowerAdvisor) {
        return false;
    }

    bool canNotify = mQtiPowerAdvisor->mSendUpdateImminent.load();
    if (mQtiPowerAdvisor->mScreenUpdateTimer) {
        mQtiPowerAdvisor->mScreenUpdateTimer->reset();
    }
    return canNotify;
}

} // namespace android::surfaceflingerextension
