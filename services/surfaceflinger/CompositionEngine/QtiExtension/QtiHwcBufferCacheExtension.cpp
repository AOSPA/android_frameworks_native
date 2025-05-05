/* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0

#include "QtiHwcBufferCacheExtension.h"
#include "../../QtiExtension/QtiExtensionContext.h"

#include <stack>
#include <cstdlib>

using android::surfaceflingerextension::QtiExtensionContext;

namespace android::compositionengineextension {

    static const constexpr size_t kMaxNumSlotsForWideVideos = 4;
    static const constexpr uint32_t MAX_VIDEO_WIDTH = 5760;
    static const constexpr uint32_t MAX_VIDEO_HEIGHT = 2160;

QtiHwcBufferCacheExtension::QtiHwcBufferCacheExtension() {
    auto sfext = QtiExtensionContext::instance().getQtiSurfaceFlingerExtn();
    if (sfext) {
        mReduceSlotsForWideVideo = sfext->qtiIsExtensionFeatureEnabled(
                                              surfaceflingerextension::kReduceSlotsForWideVideo);
    }
}

bool QtiHwcBufferCacheExtension::formatIsYuv(const PixelFormat format) {
    switch (format) {
        case HAL_PIXEL_FORMAT_YCBCR_422_SP:
        case HAL_PIXEL_FORMAT_YCRCB_420_SP:
        case HAL_PIXEL_FORMAT_YCBCR_422_I:
        case HAL_PIXEL_FORMAT_YCBCR_420_888:
        case HAL_PIXEL_FORMAT_Y8:
        case HAL_PIXEL_FORMAT_Y16:
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCBCR_P010:
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
        case HAL_PIXEL_FORMAT_NV21_ENCODEABLE:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO:
        case HAL_PIXEL_FORMAT_YCrCb_422_SP:
        case HAL_PIXEL_FORMAT_YCbCr_444_SP:
        case HAL_PIXEL_FORMAT_YCrCb_444_SP:
        case HAL_PIXEL_FORMAT_YCrCb_422_I:
        case HAL_PIXEL_FORMAT_NV21_ZSL:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP_VENUS:
        case HAL_PIXEL_FORMAT_NV12_HEIF:
        case HAL_PIXEL_FORMAT_YCbCr_420_P010_UBWC:
        case HAL_PIXEL_FORMAT_YCbCr_420_P010_VENUS:
        case HAL_PIXEL_FORMAT_CbYCrY_422_I:
        case HAL_PIXEL_FORMAT_YCbCr_422_I_10BIT:
        case HAL_PIXEL_FORMAT_YCbCr_422_I_10BIT_COMPRESSED:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS_UBWC:
        case HAL_PIXEL_FORMAT_YCbCr_420_TP10_UBWC:
            return true;
         default:
            return false;
    }
}

void QtiHwcBufferCacheExtension::ResetFreeSlotsForWideVideo(const sp<GraphicBuffer>& buffer,
                                                            HwcBufferCache* hwcBufferCache) {
    if (!mReduceSlotsForWideVideo || !buffer) { return; }
    uint32_t width = buffer->getWidth();
    uint32_t height = buffer->getHeight();
    PixelFormat format = buffer->getPixelFormat();
    if ((width * height > MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT) && formatIsYuv(format)) {
      // 8K video layer; Reset buffer slots to kMaxNumSlotsForWideVideos
      hwcBufferCache->mFreeSlots = std::stack<uint32_t>();
      hwcBufferCache->mCacheByBufferId.clear();
      ALOGV("Reseting FreeSlots for buffer id: %" PRIu64 " ", buffer->getId());
      for (uint32_t i = kMaxNumSlotsForWideVideos; i-- > 0;) {
        hwcBufferCache->mFreeSlots.push(i);
      }
    }
    hwcBufferCache->mSlotsSetForWideVideo = true;
}

} // namespace android::compositionengineextension
