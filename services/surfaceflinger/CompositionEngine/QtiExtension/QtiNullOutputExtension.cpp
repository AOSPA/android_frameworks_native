/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "QtiNullOutputExtension.h"

namespace android::compositionengineextension {

QtiNullOutputExtension::QtiNullOutputExtension(compositionengine::impl::Output* output)
      : mQtiOutput(output) {
    if (!mQtiOutput) {
        ALOGW("Invalid pointer to CompositionEngine::Output passed");
    }
}

bool QtiNullOutputExtension::qtiHasSecureContent() {
    return false;
}
bool QtiNullOutputExtension::qtiHasSecureDisplay() {
    return false;
}

} // namespace android::compositionengineextension
