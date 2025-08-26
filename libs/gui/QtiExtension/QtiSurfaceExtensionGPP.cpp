/* Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Indentifier: BSD-3-Clause-Clear
 */

#undef LOG_TAG
#define LOG_TAG "SurfaceExtension-GPP"
// #define LOG_NDEBUG 0
#include "dlfcn.h"
#include <cutils/misc.h>
#include <cutils/properties.h>
#include "QtiSurfaceExtensionGPP.h"
#include <com_android_graphics_libgui_flags.h>
#include <unordered_map>

using ::android::IGraphicBufferProducer;
using ::android::sp;
using ::android::status_t;
using ::android::IBinder;

typedef status_t (*InitFunc_t)(sp<IGraphicBufferProducer>*, const sp<IBinder>&);
typedef void (*DeinitFunc_t)(const sp<IBinder>&);
static constexpr uint32_t BQ_LAYER_COUNT = 1;

namespace android::libguiextension {
QtiSurfaceExtensionGPP::QtiSurfaceExtensionGPP(
        Surface* surface,
        const sp<IBinder> handle,
        sp<IGraphicBufferProducer>* gbp)
    : mSurface(surface),
      mIsEnable(false),
      mIsSupported(true),
      mConnectedToGpu(false),
      mOriginalGbp(*gbp),
      mGbp(nullptr),
      mHandle(handle),
      mLibHandler(nullptr),
      mFuncInit(nullptr),
      mFuncDeinit(nullptr),
      mConnectedProducerListener(),
      mClientSetBufferCount(0),
      mLastQueuedBufferSlot(-1) {
    // FIRST_APPLICATION_UID / AID_APP_START is first uid for 3rd party application.
    // The system application will not enter this logic.
    mUID = getuid();
    if (mUID < FIRST_APPLICATION_UID) {
        mIsSupported = false;
        return;
    }

    mLibHandler = dlopen("libgppextension.so", RTLD_NOW|RTLD_GLOBAL);
    if (!mLibHandler) {
        ALOGV("%s: mHandle %p, failed dlopen libgppextension, err %s",
            __FUNCTION__, mHandle.get(), dlerror());
        mIsSupported = false;
        return;
    }
    mFuncInit = dlsym(mLibHandler, "Init");
    mFuncDeinit = dlsym(mLibHandler, "Deinit");
    if (!mFuncInit || !mFuncDeinit) {
        ALOGV("%s: mHandle %p, failed dlsym functions, err %s",
            __FUNCTION__, mHandle.get(), dlerror());
        dlclose(mLibHandler);
        mLibHandler = nullptr;
        mIsSupported = false;
        return;
    }
    ALOGV("Created Surface Extension for GPP, original buffer producer = %p", mOriginalGbp.get());
}

bool QtiSurfaceExtensionGPP::Connect(int api, sp<IGraphicBufferProducer>* gbp) {
    if (api != NATIVE_WINDOW_API_EGL) {
        ALOGV("Connect api %d is not EGL. No need to enable GPP feather.", api);
        return false;
    }

    std::lock_guard _lock{mMutex};
    mConnectedToGpu = true;
    return DynamicEnableInternal(gbp, false);
}

void QtiSurfaceExtensionGPP::Disconnect(int api, sp<IGraphicBufferProducer>* gbp) {
    if (api == NATIVE_WINDOW_API_EGL || IsGPPEnabled()) {
        std::lock_guard _lock{mMutex};
        mConnectedToGpu = false;
        DisableGPPinternal(gbp);
    }
}

bool QtiSurfaceExtensionGPP::DynamicEnable(sp<IGraphicBufferProducer>* gbp) {
    std::lock_guard _lock{mMutex};
    return DynamicEnableInternal(gbp, true);
}

void QtiSurfaceExtensionGPP::DisableGPPinternal(sp<IGraphicBufferProducer>* gbp) {
    if (mIsEnable && mFuncDeinit) {
        reinterpret_cast<DeinitFunc_t>(mFuncDeinit)(mHandle);
    }
    if (mLibHandler != nullptr) {
        dlclose(mLibHandler);
    }
    mLibHandler = nullptr;
    mGbp = nullptr;
    if (mOriginalGbp != nullptr) {
        if (static_cast<uint32_t>(mFrameRate) != 0) {
            ALOGI("Change back to App set FrameRate %.1f", mFrameRate);
            mOriginalGbp->setFrameRate(mFrameRate, mCompatibility, mChangeFrameRateStrategy);
        }
        *gbp = mOriginalGbp;
        SetGraphicBufferProducer(*gbp);
        mIsEnable = false;
    } else {
        ALOGV("mOriginalGbp is not set.");
    }
}

bool QtiSurfaceExtensionGPP::DynamicEnableInternal(sp<IGraphicBufferProducer>* gbp, bool needReconnect) {
    if (mIsSupported && mConnectedToGpu) {
        char valueStr[PROPERTY_VALUE_MAX] = {0};
        property_get("vendor.gpp.frc.enable", valueStr, "0x11");//default value should not be 0x0(FRC OFF) or 0x1(FRC ON),need to other value,choose 0x11.
        int enable = -1;
        size_t pos = 0;
        int property = std::stoul(valueStr, &pos, 16);
        if (property == 0x21) {  // 0x21 dynamic off
            enable = 0;
        } else if (property == 0x22) {  // 0x22 dynamic on
            enable = 1;
        }

        ALOGV("Property enable = %d, before dynamic enable, mGraphicBufferProducer = %p ", enable, gbp->get());
        if (enable !=-1 && mIsEnable != enable) {
            if (enable) {
                ALOGV("Enabling GPP feather");
                status_t err = reinterpret_cast<InitFunc_t>(mFuncInit)(gbp, mHandle);
                if (err == OK) {
                    mIsEnable = true;
                    mIsSupported = true;
                    mGbp = *gbp;
                    if (static_cast<uint32_t>(mFrameRate) != 0 && mOriginalGbp) {
                        ALOGI("App set FrameRate %.1f, overwrite with 0.0 after GPP enabled",
                            mFrameRate);
                        mOriginalGbp->setFrameRate(0.0f, mCompatibility, mChangeFrameRateStrategy);
                    }
                } else {
                    mIsEnable = false;
                    if (err == NAME_NOT_FOUND || err == INVALID_OPERATION) {
                       mIsSupported = false;
                       ALOGV("Failed to init GPP: Surface or App is not supported by GPP");
                    } else {
                        mIsSupported = false;
                        ALOGV("Failed to init GPP: Unknown error.");
                    }
                }
            } else {
                ALOGV("Disabling GPP feather");
                DisableGPPinternal(gbp);
            }
            if (needReconnect && mIsEnable == enable && nullptr != *gbp && nullptr != mConnectedProducerListener) {
               IGraphicBufferProducer::QueueBufferOutput output;
               (*gbp)->connect(mConnectedProducerListener, mAPI, mReportBufferRemoval, &output);
               TransferBuffersToNewQueue(gbp);
            }
            if (mIsEnable == enable) {
                ALOGV("GPP dynamic On/Off succeeded, mIsEnable = %d", mIsEnable);
                return true;
            } else {
                ALOGV("GPP dynamic On/Off failed, mIsEnable = %d", mIsEnable);
                return false;
            }

        } else {
            ALOGV("No need to change BufferProducer");
            return true;
        }

        ALOGV("After dynamic enable, mGraphicBufferProducer = %p, mOriginalGbp = %p", gbp->get(), mOriginalGbp.get());
    } else {
        ALOGV("Unsupport Surface");
        return true;
    }
}

 void QtiSurfaceExtensionGPP::StoreConnect(int api, const sp<IProducerListener>& listener, bool reportBufferRemoval) {
   mAPI = api;
   mConnectedProducerListener = listener;
   mReportBufferRemoval = reportBufferRemoval;
}

 void QtiSurfaceExtensionGPP::SetGraphicBufferProducer(sp<IGraphicBufferProducer> gbp) {
    if (gbp != nullptr) {
        if (mSidebandStream.seted)
            gbp->setSidebandStream(mSidebandStream.stream);
    }
}

int QtiSurfaceExtensionGPP::query(int what, int *outValue) const {
    std::lock_guard _lock{mMutex};

    if (mOriginalGbp == nullptr) {
        ALOGE("mIsEnable = %d , mOriginalGbp must not be NULL", mIsEnable);
        return BAD_VALUE;
    }

    if (mIsEnable == true) {
        switch (what) {
            case NATIVE_WINDOW_WIDTH:
            case NATIVE_WINDOW_HEIGHT:
            case NATIVE_WINDOW_FORMAT:
            case NATIVE_WINDOW_CONSUMER_USAGE_BITS:
            case NATIVE_WINDOW_STICKY_TRANSFORM:
            case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND:
            case NATIVE_WINDOW_DEFAULT_DATASPACE:
            case NATIVE_WINDOW_BUFFER_AGE:
            case NATIVE_WINDOW_CONSUMER_IS_PROTECTED:
                // Call the mOriginalGbp directly to avoid unnecessary binder call due to those values of mGbp are the same as mOriginalGbp.
                return mOriginalGbp->query(what, outValue);
            case NATIVE_WINDOW_LAYER_COUNT:
                // All BufferQueue buffers have a single layer.
                *outValue = BQ_LAYER_COUNT;
                return NO_ERROR;
            case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
                if (mGbp != nullptr) {
                    return mGbp->query(what, outValue);
                } else {
                    ALOGW("mGbp is NULL");
                    return BAD_VALUE;
                }
            default:
                return BAD_VALUE;
        }
    } else {
        return mOriginalGbp->query(what, outValue);
    }
}

QtiSurfaceExtensionGPP::~QtiSurfaceExtensionGPP() {
    if (mIsEnable && mFuncDeinit) {
        reinterpret_cast<DeinitFunc_t>(mFuncDeinit)(mHandle);
    }
    if (mLibHandler != nullptr) {
        dlclose(mLibHandler);
    }
    mLibHandler = nullptr;
    ALOGV("~QtiSurfaceExtensionGPP()");
}

void QtiSurfaceExtensionGPP::TransferBuffersToNewQueue(sp<IGraphicBufferProducer>* gbp) {
    if (mSurface == nullptr) {
        ALOGE("NULL Surface");
        return;
    }
    int count = 0;
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_UNLIMITED_SLOTS)
    for (int slot = 0; slot < (int)mSurface->mSlots.size(); ++slot) {
#else
    for (int slot = 0; slot < Surface::NUM_BUFFER_SLOTS; ++slot) {
#endif
        if (mSurface->mSlots[slot].buffer != nullptr) {
            count++;
        }
    }
    int minUndequeuedBuffers = 0;
    int maxDequeuedBufferCount = 0;
    if (mClientSetBufferCount == 0) {
        maxDequeuedBufferCount = 1;
    } else {
        if ((*gbp)->query(
            NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minUndequeuedBuffers) == NO_ERROR) {
            maxDequeuedBufferCount = mClientSetBufferCount - minUndequeuedBuffers;
        } else {
            maxDequeuedBufferCount = 2;
        }
    }
    ALOGI("Min undequeued count %d, max dequeued count %d",
        minUndequeuedBuffers, maxDequeuedBufferCount);
    if ((count == 0) || (count != mClientSetBufferCount)) {
        ALOGI("Free buffer count %d mismatch client set buffer count %d",
            count, mClientSetBufferCount);
        (*gbp)->allowAllocation(true);
        (*gbp)->setMaxDequeuedBufferCount(maxDequeuedBufferCount);
        return;
    }
    // App should only uses existing buffers rather than dequeue any new
    // slot and request new buffers for dynamic ON/OFF case
    (*gbp)->allowAllocation(false);
    (*gbp)->setMaxDequeuedBufferCount(maxDequeuedBufferCount);
    ALOGI("DisAllow dequeue new slots and set max dequeued buffer count %d",
        maxDequeuedBufferCount);

    std::unordered_map<int, sp<GraphicBuffer>> newSlotsToBuffersMapping {};
    // TODO: Check if we need acquire lock when operate on mSlots
	int lastQueuedBufferSlot = -1;
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_UNLIMITED_SLOTS)
    for (int slot = 0; slot < (int)mSurface->mSlots.size(); ++slot) {
#else
    for (int slot = 0; slot < Surface::NUM_BUFFER_SLOTS; ++slot) {
#endif
        if (mSurface->mSlots[slot].buffer != nullptr) {
            if (slot == mLastQueuedBufferSlot) {
                lastQueuedBufferSlot = mLastQueuedBufferSlot;
                ALOGI("mLastQueuedBufferSlot: %d", mLastQueuedBufferSlot);
                continue;
            }
            int newSlot = -1;
            status_t result = (*gbp)->attachBuffer(&newSlot, mSurface->mSlots[slot].buffer);
            if (result == NO_ERROR && newSlot != -1) {
                newSlotsToBuffersMapping.emplace(newSlot, std::move(mSurface->mSlots[slot].buffer));
                ALOGI("Same buffer %p is mapped from old slot=%d to new slot=%d",
                    &(mSurface->mSlots[slot].buffer), slot, newSlot);
                (*gbp)->cancelBuffer(newSlot, Fence::NO_FENCE);
            }
        }
    }

    if (lastQueuedBufferSlot >= 0) {
        int slot = lastQueuedBufferSlot;
        int newSlot = -1;
        status_t result = (*gbp)->attachBuffer(&newSlot, mSurface->mSlots[slot].buffer);
        if (result == NO_ERROR && newSlot != -1) {
            newSlotsToBuffersMapping.emplace(newSlot, std::move(mSurface->mSlots[slot].buffer));
            ALOGI("Same buffer %p is mapped from old slot=%d to new slot=%d",
                &(mSurface->mSlots[slot].buffer), slot, newSlot);
            (*gbp)->cancelBuffer(newSlot, Fence::NO_FENCE);
        }
    }

    for (auto& [newSlot, buffer] : newSlotsToBuffersMapping) {
        mSurface->mSlots[newSlot].buffer = std::move(buffer);
    }
    newSlotsToBuffersMapping.clear();
}

void QtiSurfaceExtensionGPP::setFrameRate(float frameRate, int8_t compatibility,
    int8_t changeFrameRateStrategy) {
    ALOGI("FrameRate %.1f, compatibility %d changeFrameRateStrategy %d",
        frameRate, compatibility, changeFrameRateStrategy);
    mFrameRate = frameRate;
    mCompatibility = compatibility;
    mChangeFrameRateStrategy = changeFrameRateStrategy;
}

} //namespace android::libguiextension