/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#define LOG_NDEBUG 0
#include "QtiAidlComposerHalExtension.h"

namespace android::surfaceflingerextension {

QtiAidlComposerHalExtension::QtiAidlComposerHalExtension(Hwc2::Composer* composerHal) {
    if (!composerHal) {
        ALOGW("Passed an invalid pointer to composer hal");
        return;
    }

    mQtiAidlComposer = static_cast<Hwc2::AidlComposer*>(composerHal);
    ALOGV("Successfully created QtiAidlComposerHalExtension %p", mQtiAidlComposer);
}

Error QtiAidlComposerHalExtension::qtiSetDisplayElapseTime(Display display, uint64_t timeStamp) {
    Error error = Error::NONE;
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    if (mQtiAidlComposer->getWriter(display)) {
        mQtiAidlComposer->getWriter(display)->get().qtiSetDisplayElapseTime(static_cast<int64_t>(
                                                                                    display),
                                                                            timeStamp);
    } else {
        error = Error::BAD_DISPLAY;
        ALOGI("%s: Attempted to set display elapsed time for disconnected display %" PRId64,
              __func__, display);
    }
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return error;
}

Error QtiAidlComposerHalExtension::qtiSetLayerType(Display display, V2_1_Layer layer,
                                                   uint32_t type) {
    Error error = Error::NONE;
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    if (mQtiAidlComposer->getWriter(display)) {
        mQtiAidlComposer->getWriter(display)->get().qtiSetLayerType(static_cast<int64_t>(display),
                                                                    static_cast<int64_t>(layer),
                                                                    type);
    } else {
        error = Error::BAD_DISPLAY;
        ALOGI("%s: Attempted to set layer type for disconnected display %" PRId64, __func__,
              display);
    }
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return error;
}

Error QtiAidlComposerHalExtension::qtiSetLayerFlag(Display display, V2_1_Layer layer,
                                                   uint32_t flags) {
    Error error = Error::NONE;
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    if (mQtiAidlComposer->getWriter(display)) {
        mQtiAidlComposer->getWriter(display)->get().qtiSetLayerFlag(static_cast<int64_t>(display),
                                                                    static_cast<int64_t>(layer),
                                                                    static_cast<QtiLayerFlags>(
                                                                            flags));

    } else {
        error = Error::BAD_DISPLAY;
        ALOGI("%s: Attempted to set layer flag for disconnected display %" PRId64, __func__,
              display);
    }
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return error;
}

Error QtiAidlComposerHalExtension::qtiSetClientTarget_3_1(Display display, int32_t slot,
                                                          int acquireFence,
                                                          uint32_t dataspace) {
    Error error = Error::NONE;
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    if (mQtiAidlComposer->getWriter(display)) {
        mQtiAidlComposer->getWriter(display)->get().qtiSetClientTarget_3_1(static_cast<int64_t>(
                                                                                   display),
                                                                           static_cast<uint32_t>(
                                                                                   slot),
                                                                           acquireFence, dataspace);
    } else {
        error = Error::BAD_DISPLAY;
        ALOGI("%s: Attempted to set client target for disconnected display %" PRId64, __func__,
              display);
    }
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return error;
}

Error QtiAidlComposerHalExtension::qtiTryDrawMethod(Display display,
                                                    uint32_t drawMethod) {
    Error ret = Error::NONE;
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    if (mQtiAidlComposer->qtiComposer3Client) {
        auto status = mQtiAidlComposer->qtiComposer3Client
                              ->qtiTryDrawMethod(static_cast<int64_t>(display),
                                                 static_cast<QtiDrawMethod>(drawMethod));
        if (!status.isOk()) {
            ALOGE("tryDrawMethod failed %s", status.getDescription().c_str());
            ret = static_cast<Error>(status.getServiceSpecificError());
        }
    }
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return ret;
}

} // namespace android::surfaceflingerextension
