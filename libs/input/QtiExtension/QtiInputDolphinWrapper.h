/* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

namespace android {

class QtiInputDolphinWrapper {
public:
    QtiInputDolphinWrapper();
    ~QtiInputDolphinWrapper();
    static QtiInputDolphinWrapper* qtiGetDolphinWrapper();
    static QtiInputDolphinWrapper* qtiGetInstance();
    void (*qtiDolphinSetTouchEvent)(int eventType) = nullptr;
    void (*qtiDolphinConsumeInputNow)(bool& consumeBatches) = nullptr;

private:
    void *mQtiDolphinHandle = nullptr;
    static QtiInputDolphinWrapper* sInstance;
};

} // namespace android
