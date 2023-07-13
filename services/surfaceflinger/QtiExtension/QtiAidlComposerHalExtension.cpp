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
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    mQtiAidlComposer->getWriter(display)->get().qtiSetDisplayElapseTime(static_cast<int64_t>(
                                                                                display),
                                                                        timeStamp);
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return Error::NONE;
}

Error QtiAidlComposerHalExtension::qtiSetLayerType(Display display, V2_1_Layer layer,
                                                   uint32_t type) {
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    mQtiAidlComposer->getWriter(display)->get().qtiSetLayerType(static_cast<int64_t>(display),
                                                                static_cast<int64_t>(layer), type);
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return Error::NONE;
}

Error QtiAidlComposerHalExtension::qtiSetLayerFlag(Display display, V2_1_Layer layer,
                                                   uint32_t flags) {
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    mQtiAidlComposer->getWriter(display)->get().qtiSetLayerFlag(static_cast<int64_t>(display),
                                                                static_cast<int64_t>(layer),
                                                                static_cast<QtiLayerFlags>(flags));
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return Error::NONE;
}

Error QtiAidlComposerHalExtension::qtiSetClientTarget_3_1(Display display, int32_t slot,
                                                          int acquireFence,
                                                          uint32_t dataspace) {
#ifdef QTI_COMPOSER3_EXTENSIONS
    mQtiAidlComposer->mMutex.lock_shared();
    mQtiAidlComposer->getWriter(display)->get().qtiSetClientTarget_3_1(static_cast<int64_t>(
                                                                               display),
                                                                       static_cast<uint32_t>(slot),
                                                                       acquireFence, dataspace);
    mQtiAidlComposer->mMutex.unlock_shared();
#endif
    return Error::NONE;
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
