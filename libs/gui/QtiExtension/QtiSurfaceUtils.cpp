/* Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0

#include <utils/Log.h>

#include "QtiSurfaceUtils.h"

namespace android::libguiextension {

static std::mutex sPerfServiceMutex;
static sp<IBinder> sPerfBinder = nullptr;

sp<IBinder> getPerfService() {
    std::lock_guard<std::mutex> lock(sPerfServiceMutex);

    if (sPerfBinder == nullptr) {
        sp<IServiceManager> sm = defaultServiceManager();
        sPerfBinder = sm->checkService(String16("vendor.perfservice"));
    }
    return sPerfBinder;
}

} // namespace android