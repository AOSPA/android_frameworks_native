/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "QtiSurfaceFlingerExtensionFactory.h"
#include <android-base/properties.h>

#ifdef QTI_DISPLAY_EXTENSION
#include "QtiVirtualDisplaySurfaceExtension.h"
#include "QtiFramebufferSurfaceExtension.h"
#include "QtiSurfaceFlingerExtension.h"
#endif

namespace android::surfaceflingerextension {

QtiSurfaceFlingerExtensionIntf* qtiCreateSurfaceFlingerExtension(SurfaceFlinger* flinger) {
#ifdef QTI_DISPLAY_EXTENSION
    int qtiFirstApiLevel = android::base::GetIntProperty("ro.product.first_api_level", 0);
    bool mQtiEnableDisplayExtn = (qtiFirstApiLevel < __ANDROID_API_U__) ||
            base::GetBoolProperty("vendor.display.enable_display_extensions", false);
    if (mQtiEnableDisplayExtn) {
        ALOGI("Enabling QtiSurfaceFlingerExtension ...");
        return new QtiSurfaceFlingerExtension();
    }
#endif

    ALOGI("Enabling QtiNullSurfaceFlingerExtension in QSSI ...");
    return new QtiNullExtension(flinger);
}

QtiDisplaySurfaceExtensionIntf* qtiCreateDisplaySurfaceExtension(bool isVirtual,
                                                                 VirtualDisplaySurface* vds,
                                                                 bool secure, uint64_t sinkUsage,
                                                                 FramebufferSurface* fbs) {
#ifdef QTI_DISPLAY_EXTENSION
    int qtiFirstApiLevel = android::base::GetIntProperty("ro.product.first_api_level", 0);
    bool mQtiEnableDisplayExtn = (qtiFirstApiLevel < __ANDROID_API_U__) ||
            base::GetBoolProperty("vendor.display.enable_display_extensions", false);
    if (mQtiEnableDisplayExtn) {
        if (isVirtual) {
            ALOGV("Enabling QtiVirtualDisplaySurfaceExtension ...");
            return new QtiVirtualDisplaySurfaceExtension(vds, secure, sinkUsage);
        }
        return new QtiFramebufferSurfaceExtension(fbs);
    }
#endif

    ALOGI("Enabling QtiNullDisplaySurfaceExtension in QSSI ...");
    return new QtiNullDisplaySurfaceExtension(vds, secure, sinkUsage);
}

} // namespace android::surfaceflingerextension
