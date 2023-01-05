/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiOutputExtension.h"

#define LOG_TAG "QtiCompositionEngineExtension"
#include <log/log.h>

namespace android::compositionengineextension {

QtiOutputExtension::QtiOutputExtension(compositionengine::impl::Output* output)
      : mQtiOutput(output) {
    if (!mQtiOutput) {
        ALOGW("Invalid pointer to Output passed.");
        return;
    }

    ALOGV("Successfully created QtiOutputExtension %p", mQtiOutput);
}

bool QtiOutputExtension::qtiHasSecureContent() {
    if (!mQtiOutput) {
        return false;
    }

    bool qtiHasSecureCamera = false;
    bool qtiHasSecureDisplay = false;
    bool qtiNeedsProtected = false;

    for (auto* layer : mQtiOutput->getOutputLayersOrderedByZ()) {
        if (layer->getLayerFE().getCompositionState()->qtiIsSecureCamera) {
            qtiHasSecureCamera = true;
        }
        if (layer->getLayerFE().getCompositionState()->qtiIsSecureDisplay) {
            qtiHasSecureDisplay = true;
        }
        if (layer->getLayerFE().getCompositionState()->hasProtectedContent) {
            qtiNeedsProtected = true;
        }
    }

    return !qtiHasSecureCamera && !qtiHasSecureDisplay && qtiNeedsProtected;
}

bool QtiOutputExtension::qtiHasSecureDisplay() {
    if (!mQtiOutput) {
        return false;
    }
    auto layers = mQtiOutput->getOutputLayersOrderedByZ();
    bool hasSecureDisplay = std::any_of(layers.begin(), layers.end(), [](auto* layer) {
        return layer->getLayerFE().getCompositionState()->qtiIsSecureDisplay;
    });

    return hasSecureDisplay;
}

} // namespace android::compositionengineextension
