/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Indentifier: BSD-3-Clause-Clear
 */
#include "QtiImageConsumerExtension.h"

#include <android-base/properties.h>
#include <dlfcn.h>
#include <log/log.h>

namespace android::libnativedisplay {

QtiImageConsumerExtension::QtiImageConsumerExtension(ImageConsumer* consumer)
      : mQtiImageConsumer(consumer) {
#ifdef QTI_DISPLAY_EXTENSION
      int qtiFirstApiLevel = android::base::GetIntProperty("ro.product.first_api_level", 0);
      mQtiEnableExtn = (qtiFirstApiLevel < __ANDROID_API_U__) ||
              base::GetBoolProperty("vendor.display.enable_display_extensions", false);
      if (mQtiEnableExtn) {
          if (!mQtiImageConsumer) {
              ALOGW("%s: Invalid pointer to ImageConsumer passed", __func__);
          } else {
              ALOGV("%s: ImageConsumer %p", __func__, mQtiImageConsumer);
          }
      }
#endif

}

void QtiImageConsumerExtension::updateBufferDataSpace(
        const sp<GraphicBuffer> graphicBuffer, BufferItem& item) {
    if (!mQtiEnableExtn || !mQtiImageConsumer) {
        return;
    }

    ui::Dataspace qtiDataspace;
    if (graphicBuffer->qtiGetDataspace(&qtiDataspace) == 0) {
        if (qtiDataspace != ui::Dataspace::UNKNOWN) {
            item.mDataSpace = static_cast<android_dataspace>(qtiDataspace);
        }
    }
}

} //namespace android::libnativedisplay
