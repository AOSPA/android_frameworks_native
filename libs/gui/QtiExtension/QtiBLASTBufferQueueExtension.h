/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../include/gui/BLASTBufferQueue.h"

namespace android {

namespace libguiextension {

class QtiBLASTBufferQueueExtension {
public:
    QtiBLASTBufferQueueExtension(BLASTBufferQueue* blastBufferQueue, const std::string& name);
    ~QtiBLASTBufferQueueExtension() = default;

    void qtiSetConsumerUsageBitsForRC(std::string name, sp<SurfaceControl> sc);
    bool qtiIsGame();
    void qtiTrackTransaction(uint64_t frameNumber, int64_t timestamp);
    void qtiSendGfxTid();

private:
    BLASTBufferQueue* mQtiBlastBufferQueue = nullptr;
};

} // namespace libguiextension
} // namespace android
