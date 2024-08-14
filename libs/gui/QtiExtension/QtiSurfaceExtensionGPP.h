/* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <utils/RefBase.h>
#include "../include/gui/Surface.h"

namespace android {

namespace libguiextension {

class QtiSurfaceExtensionGPP {
public:
    QtiSurfaceExtensionGPP(const sp<IBinder> handle, sp<IGraphicBufferProducer>* gbp);
    bool Connect(int api, sp<IGraphicBufferProducer>* gbp);
    void Disconnect(int api, sp<IGraphicBufferProducer>* gbp);
    bool DynamicEnable(sp<IGraphicBufferProducer>* gbp);
    void StoreConnect(int api, const sp<IProducerListener>& listener,bool reportBufferRemoval);
    inline bool IsGPPEnabled() const { return mIsEnable;};
    inline bool IsGPPSupported() const { return mIsSupported && mConnectedToGpu;};
    int getUid() const { return mUID;};
    int query(int what, int* outValue) const;

    struct SidebandStream
    {
       bool seted = false;
       sp<NativeHandle> stream = nullptr;
    };
    inline void setSidebandStream(const sp<NativeHandle>& stream) {
       mSidebandStream.seted = true;
       mSidebandStream.stream = stream;
    };
    ~QtiSurfaceExtensionGPP();
private:
    bool mIsEnable;
    bool mIsSupported;
    bool mConnectedToGpu;
    int mUID;
    sp<IGraphicBufferProducer> mOriginalGbp;
    sp<IGraphicBufferProducer> mGbp;
    sp<IBinder> mHandle;
    void* mLibHandler;
    void* mFuncInit;
    void* mFuncDeinit;
    SidebandStream mSidebandStream;
    int mAPI;
    sp<IProducerListener> mConnectedProducerListener;
    bool mReportBufferRemoval;
    mutable std::mutex mMutex;
    void DisableGPPinternal(sp<IGraphicBufferProducer>* gbp);
    bool DynamicEnableInternal(sp<IGraphicBufferProducer>* gbp, bool needReconnect);
    void SetGraphicBufferProducer(sp<IGraphicBufferProducer> gbp);
};

} // namespace libguiextension
} // namespace android

