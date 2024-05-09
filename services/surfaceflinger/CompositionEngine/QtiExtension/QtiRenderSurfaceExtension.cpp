/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0

#include "QtiRenderSurfaceExtension.h"

#include <android-base/properties.h>
#include <system/window.h>
#include "compositionengine/CompositionEngine.h"

using android::compositionengine::CompositionEngine;

struct ANativeWindow;

namespace android::compositionengineextension {

QtiRenderSurfaceExtension::QtiRenderSurfaceExtension(RenderSurface* renderSurface)
      : mQtiRenderSurface(renderSurface) {
#ifdef QTI_DISPLAY_EXTENSION
    int qtiFirstApiLevel = android::base::GetIntProperty("ro.product.first_api_level", 0);
    mQtiEnableExtn = (qtiFirstApiLevel < __ANDROID_API_U__) ||
            base::GetBoolProperty("vendor.display.enable_display_extensions", false);
    if (mQtiEnableExtn) {
        if (!mQtiRenderSurface) {
            ALOGW("%s: Invalid pointer to RenderSurface passed", __func__);
        } else {
            ALOGV("%s: RenderSurface %p", __func__, mQtiRenderSurface);
        }
        ALOGI("Enabling QtiRenderSurfaceExtension ...");
    }
#endif
}

int32_t QtiRenderSurfaceExtension::qtiGetClientTargetFormat() {
    // When QtiExtensions are disabled or when pointer to RenderSurface is invalid, return a
    // default value
    if (!mQtiEnableExtn || !mQtiRenderSurface) {
        return -1;
    }

    return ANativeWindow_getFormat(mQtiRenderSurface->mNativeWindow.get());
}

void QtiRenderSurfaceExtension::qtiSetViewportAndProjection() {
    Rect sourceCrop = Rect(mQtiRenderSurface->mSize);
    Rect viewPort = Rect(mQtiRenderSurface->mSize.width, mQtiRenderSurface->mSize.height);
    auto& renderEngine = mQtiRenderSurface->mCompositionEngine.getRenderEngine();
    renderEngine.setViewportAndProjection(viewPort, sourceCrop);
}

} // namespace android::compositionengineextension
