/* Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#ifndef QTISURFACEUTILS_H
#define QTISURFACEUTILS_H

#include <binder/IServiceManager.h>
#include <mutex>
#include <utils/Log.h>

namespace android::libguiextension {

sp<IBinder> getPerfService();

} // namespace android

#endif // QTISURFACEUTILS_H