/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0

#include "QtiOutputExtension.h"
#include "QtiNullOutputExtension.h"

#define LOG_TAG "QtiCompositionEngineExtension"

#include <android-base/properties.h>
#include <log/log.h>

namespace android::compositionengineextension {

QtiOutputExtensionIntf* qtiCreateOutputExtension(compositionengine::impl::Output* output) {
#ifdef QTI_DISPLAY_EXTENSION
    bool mQtiEnableDisplayExtn =
            base::GetBoolProperty("vendor.display.enable_display_extensions", false);
    if (mQtiEnableDisplayExtn) {
        ALOGV("Enabling QtiOutputExtension ...");
        return new QtiOutputExtension(output);
    }
#endif

    ALOGV("Enabling QtiNullOutputExtension in QSSI ...");
    return new QtiNullOutputExtension(output);
}

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

    bool hasSecureDisplay = false;
    for (auto* outputLayer : mQtiOutput->getOutputLayersOrderedByZ()) {
        if (!outputLayer || !outputLayer->getLayerFE().getCompositionState()) {
            ALOGV("Avoid isSecureDisplay check - outputlayer or layer.compositionState is null");
            continue;
        }

        if (outputLayer->getLayerFE().getCompositionState()->qtiIsSecureDisplay) {
            hasSecureDisplay = true;
            break;
        }
    }

    return hasSecureDisplay;
}

} // namespace android::compositionengineextension
