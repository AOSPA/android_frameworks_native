/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0

#include "QtiOutputExtension.h"
#include "../../QtiExtension/QtiExtensionContext.h"
#include "aidl/android/hardware/graphics/common/DisplayDecorationSupport.h"
#include "aidl/android/hardware/graphics/composer3/DisplayCapability.h"

#define LOG_TAG "QtiCompositionEngineExtension"
#include <log/log.h>

using android::surfaceflingerextension::QtiExtensionContext;

namespace android::compositionengineextension {

bool QtiOutputExtension::qtiIsProtectedContent(const compositionengine::impl::Output* output) {
    if (!output) {
        return false;
    }

    bool qtiHasSecureCamera = false;
    bool qtiHasSecureDisplay = false;
    bool qtiNeedsProtected = false;

    for (auto* layer : output->getOutputLayersOrderedByZ()) {
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

bool QtiOutputExtension::qtiHasSecureDisplay(const compositionengine::impl::Output* output) {
    if (!output) {
        return false;
    }

    bool hasSecureDisplay = false;
    for (auto* outputLayer : output->getOutputLayersOrderedByZ()) {
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

bool QtiOutputExtension::qtiHasSecureOrProtectedContent(const compositionengine::impl::Output* output) {
    if (!output) {
        return false;
    }

    bool qtiHasSecureCamera = false;
    bool qtiHasSecureDisplay = false;
    bool qtiNeedsProtected = false;

    for (auto* layer : output->getOutputLayersOrderedByZ()) {
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

    return qtiHasSecureCamera || qtiHasSecureDisplay || qtiNeedsProtected;
}


void QtiOutputExtension::qtiWriteLayerFlagToHWC(HWC2::Layer* layer, const Output* output) {
    uint32_t layerFlag = 0;
    bool secure = qtiHasSecureOrProtectedContent(output);
    if (!secure) {
        layerFlag = 1;
    }
    auto hwcextn = QtiExtensionContext::instance().getQtiHWComposerExtension();
    if (hwcextn) {
        hwcextn->qtiSetLayerFlag(layer, layerFlag);
    }
}

void QtiOutputExtension::qtiSetLayerAsMask(DisplayId id, uint64_t layerId) {
    const auto physicalDisplayId = PhysicalDisplayId::tryCast(id);
    auto ce =  QtiExtensionContext::instance().getCompositionEngine();

    if (ce && physicalDisplayId) {
        auto& hwc = ce->getHwComposer();
        const auto hwcDisplayId = hwc.fromPhysicalDisplayId(*physicalDisplayId);
        std::optional<aidl::android::hardware::graphics::common::DisplayDecorationSupport>
                outSupport;
        hwc.getDisplayDecorationSupport(*physicalDisplayId, &outSupport);
        if (outSupport) {
            // The HWC already supports screen decoration layers
            return;
        }

        auto sfext = QtiExtensionContext::instance().getQtiSurfaceFlingerExtn();
        if (sfext) {
            sfext->qtiSetLayerAsMask(static_cast<uint32_t>(*hwcDisplayId),layerId);
        }
    }
}

void QtiOutputExtension::qtiSetLayerType(HWC2::Layer* layer, uint32_t type,
                                                    const char* debugName __unused) {
    auto hwcextn = QtiExtensionContext::instance().getQtiHWComposerExtension();
    if (hwcextn) {
        hwcextn->qtiSetLayerType(layer, type);
    }
}

bool QtiOutputExtension::qtiUseSpecFence(void) {
    auto sfext = QtiExtensionContext::instance().getQtiSurfaceFlingerExtn();
    if (sfext) {
        return sfext->qtiIsExtensionFeatureEnabled(surfaceflingerextension::kSpecFence);
    }
    return false;
}

void QtiOutputExtension::qtiGetVisibleLayerInfo(
        const compositionengine::impl::Output* output) {
    if (!output) {
        return;
    }

    auto sfext =  QtiExtensionContext::instance().getQtiSurfaceFlingerExtn();
    if (sfext) {
        auto displayId = output->getDisplayId();
        if (!displayId.has_value()) {
            return;
        }

        for (auto* layer: output->getOutputLayersOrderedByZ()) {
            sfext->qtiSetVisibleLayerInfo(*displayId, layer->getLayerFE().getDebugName(),
                    layer->getLayerFE().getLayerId());
        }
    }
}

} // namespace android::compositionengineextension
