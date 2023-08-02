/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Indentifier: BSD-3-Clause-Clear
 */
#include "QtiEglConsumerExtension.h"

#include <android-base/properties.h>
#include <dlfcn.h>
#include <log/log.h>

namespace android::libnativedisplay {

bool QtiEglImageExtension::mQtiEnableExtn = QtiEglImageExtension::extensionEnabled();

bool QtiEglImageExtension::extensionEnabled() {
#ifdef QTI_DISPLAY_EXTENSION
    int qtiFirstApiLevel = android::base::GetIntProperty("ro.product.first_api_level", 0);
    bool enable = (qtiFirstApiLevel < __ANDROID_API_U__) ||
            base::GetBoolProperty("vendor.display.enable_display_extensions", false);
    return enable;
#else
    return false;
#endif

}

QtiEglImageExtension::QtiEglImageExtension(EGLConsumer::EglImage* image)
      : mQtiEglImage(image) {
}

bool QtiEglImageExtension::dataSpaceChanged() {
    if (!mQtiEnableExtn || !mQtiEglImage) {
        return false;
    }

    ui::Dataspace dataspace;
    if (mQtiEglImage->graphicBuffer()->qtiGetDataspace(&dataspace) == 0) {
        if (mQtiDataSpace != dataspace) {
            ALOGI("EglImage dataspace changed, need recreate");
            return true;
        }
    }

    return false;
}

void QtiEglImageExtension::setDataSpace() {
    if (!mQtiEnableExtn || !mQtiEglImage) {
        return;
    }

    ui::Dataspace dataspace;
    if (mQtiEglImage->graphicBuffer()->qtiGetDataspace(&dataspace) == 0) {
        mQtiDataSpace = dataspace;
    }
}

} //namespace android::libnativedisplay
