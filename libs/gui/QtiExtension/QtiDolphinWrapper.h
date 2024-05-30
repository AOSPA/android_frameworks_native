/* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <utils/Timers.h>

namespace android {

class QtiDolphinWrapper {
public:
    static QtiDolphinWrapper* qtiGetDolphinWrapper();
    static QtiDolphinWrapper* qtiGetInstanceForGame();
    QtiDolphinWrapper();
    ~QtiDolphinWrapper();
    void (*qtiDolphinAppInit)() = nullptr;
    bool (*qtiDolphinSmartTouchActive)() = nullptr;
    void (*qtiDolphinQueueBuffer)(bool) = nullptr;
    void (*qtiDolphinFilterBuffer)(bool& isAutoTimestamp, nsecs_t& desiredPresentTime,
                                   uint32_t& flags) = nullptr;

private:
    void *mQtiDolphinHandle = nullptr;
    static QtiDolphinWrapper* sInstance;
};

} // namespace android
