/*
 **
 ** Copyright 2012 The Android Open Source Project
 **
 ** Licensed under the Apache License Version 2.0(the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing software
 ** distributed under the License is distributed on an "AS IS" BASIS
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

// #define LOG_NDEBUG 0

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <com_android_graphics_libgui_flags.h>
#include <gui/BufferItem.h>
#include <gui/BufferItemConsumer.h>
#include <gui/BufferQueue.h>
#include <gui/Surface.h>
#include <hardware/hardware.h>
#include <log/log.h>
#include <ui/DebugUtils.h>
#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>
#include <utils/Mutex.h>
#include <utils/String8.h>

#include "../SurfaceFlinger.h"
#include "FramebufferSurface.h"
#include "HWComposer.h"
#include "HwcSlotTracker.h"
#include "MutexUtils.h"

// QTI_BEGIN: 2023-05-29: Display: sf: extensions: Clean up
#include "../QtiExtension/QtiSurfaceFlingerExtensionFactory.h"

// QTI_END: 2023-05-29: Display: sf: extensions: Clean up
namespace android {

using ui::Dataspace;

FramebufferSurface::FramebufferSurface(HWComposer& hwc, PhysicalDisplayId displayId,
                                       const ui::Size& size, const ui::Size& maxSize)
      : mDisplayId(displayId), mMaxSize(maxSize), mLimitedSize(limitSize(size)), mHwc(hwc) {
    ALOGV("Creating for display %s", to_string(displayId).c_str());

    std::tie(mRendererConsumer, mRendererSurface) = BufferItemConsumer::create(
            GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER);

// QTI_BEGIN
    if (!mQtiDSExtnIntf) {
        mQtiDSExtnIntf = surfaceflingerextension::
                qtiCreateDisplaySurfaceExtension(/* isVirtual */ false, nullptr, false, 0,
                                                 /* FramebufferSurface */ this);
    }
// QTI_END

    mRendererConsumer->setName(String8("FramebufferSurface"));
    mRendererConsumer->setDefaultBufferSize(mLimitedSize.width, mLimitedSize.height);
    mRendererConsumer->setMaxAcquiredBufferCount(SurfaceFlinger::maxFrameBufferAcquiredBuffers - 1);
}

void FramebufferSurface::onFirstRef() {
    mRendererConsumer->setBufferFreedListener(
            wp<BufferItemConsumer::BufferFreedListener>::fromExisting(this));
}

void FramebufferSurface::onBufferFreed(const sp<GraphicBuffer>& graphicBuffer) {
    Mutex::Autolock lock(mMutex);
    onBufferFreedLocked(graphicBuffer);
}

void FramebufferSurface::onBufferFreedLocked(const sp<GraphicBuffer>& graphicBuffer) {
    mHwcSlotTracker.remove(graphicBuffer);
}

void FramebufferSurface::resizeBuffers(const ui::Size& newSize) {
    const auto limitedSize = limitSize(newSize);
    mRendererConsumer->setDefaultBufferSize(limitedSize.width, limitedSize.height);
}

status_t FramebufferSurface::beginFrame(bool /*mustRecompose*/) {
    return NO_ERROR;
}

status_t FramebufferSurface::prepareFrame(CompositionType /*compositionType*/) {
    return NO_ERROR;
}

status_t FramebufferSurface::advanceFrame(float hdrSdrRatio) {
    Mutex::Autolock lock(mMutex);
    BufferItem item;
    status_t err = mRendererConsumer->acquireBuffer(&item, 0, /*waitForFence*/ false,
                                                    [&](const sp<GraphicBuffer>& buffer) {
                                                        ASSERT_MUTEX(mMutex);
                                                        onBufferFreedLocked(buffer);
                                                    });
    if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
// QTI_BEGIN: 2023-06-20: Display: sf: Fix spec fence for SDM caching
        // mDataspace = Dataspace::UNKNOWN;
// QTI_END: 2023-06-20: Display: sf: Fix spec fence for SDM caching
        return NO_ERROR;
    } else if (err != NO_ERROR) {
        ALOGE("error acquiring buffer: %s (%d)", strerror(-err), err);
// QTI_BEGIN: 2023-06-20: Display: sf: Fix spec fence for SDM caching
        // mDataspace = Dataspace::UNKNOWN;
// QTI_END: 2023-06-20: Display: sf: Fix spec fence for SDM caching
        return err;
    }

    if (mFrameData && mFrameData->mBuffer != item.mGraphicBuffer) {
        mPreviousFrameData.swap(mFrameData);
    }
    mFrameData = {item.mGraphicBuffer, item.mFence};
    mDataspace = static_cast<Dataspace>(item.mDataSpace);

    // If HWC hasn't seen the buffer before, or the buffer has changed, send it to HWC. Otherwise,
    // all we need is the slot itself.
    auto [requiresRefresh, hwcSlot] = mHwcSlotTracker.getSlot(mFrameData->mBuffer);
    sp<GraphicBuffer> hwcBuffer = requiresRefresh ? mFrameData->mBuffer : nullptr;

    status_t result = mHwc.setClientTarget(mDisplayId, hwcSlot, mFrameData->mFence, hwcBuffer,
                                           mDataspace, hdrSdrRatio);
    if (result != NO_ERROR) {
        ALOGE("error posting framebuffer: %s (%d)", strerror(-result), result);
        return result;
    }

    return NO_ERROR;
}

void FramebufferSurface::onFrameCommitted() {
    Mutex::Autolock lock(mMutex);
    // We only release the frame once an entire new frame has replaced it. This is because the frame
    // will be alive in the display hardware for panel refreshes until another has replaced it.
    if (mPreviousFrameData) {
        sp<Fence> fence =
                Fence::merge("FramebufferSurface-Acquire+Present", mPreviousFrameData->mFence,
                             mHwc.getPresentFence(mDisplayId));
        status_t result = mRendererConsumer->releaseBuffer(mPreviousFrameData->mBuffer, fence,
                                                           [&](const sp<GraphicBuffer>& buffer) {
                                                               ASSERT_MUTEX(mMutex);
                                                               onBufferFreedLocked(buffer);
                                                           });
        ALOGE_IF(result != NO_ERROR,
                 "onFrameCommitted: error releasing buffer:"
                 " %s (%d)",
                 strerror(-result), result);

        mPreviousFrameData.reset();
    }
}

ui::Size FramebufferSurface::limitSize(const ui::Size& size) {
    return limitSizeInternal(size, mMaxSize);
}

ui::Size FramebufferSurface::limitSizeInternal(const ui::Size& size, const ui::Size& maxSize) {
    ui::Size limitedSize = size;
    bool wasLimited = false;
    if (size.width > maxSize.width && maxSize.width != 0) {
        const float aspectRatio = static_cast<float>(size.width) / size.height;
        limitedSize.height = maxSize.width / aspectRatio;
        limitedSize.width = maxSize.width;
        wasLimited = true;
    }
    if (limitedSize.height > maxSize.height && maxSize.height != 0) {
        const float aspectRatio = static_cast<float>(size.width) / size.height;
        limitedSize.height = maxSize.height;
        limitedSize.width = maxSize.height * aspectRatio;
        wasLimited = true;
    }
    ALOGI_IF(wasLimited, "Framebuffer size has been limited to [%dx%d] from [%dx%d]",
             limitedSize.width, limitedSize.height, size.width, size.height);
    return limitedSize;
}

void FramebufferSurface::dumpAsString(String8& result) const {
    Mutex::Autolock lock(mMutex);
    result.append("   FramebufferSurface\n");
    result.appendFormat("      mDataspace=%s (%d)\n",
                        dataspaceDetails(static_cast<android_dataspace>(mDataspace)).c_str(),
                        mDataspace);
    mRendererConsumer->dumpState(result);
}

const sp<Fence>& FramebufferSurface::getClientTargetAcquireFence() const {
    return mFrameData ? mFrameData->mFence : Fence::NO_FENCE;
}

} // namespace android

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion"
