/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <unordered_map>
#include "../Scheduler/VsyncConfiguration.h"

using std::pair;
using std::unordered_map;

namespace android::scheduler {
class VsyncConfiguration;
class WorkDuration;
} // namespace android::scheduler

namespace android::scheduler::impl {
class WorkDuration;
}

namespace android {
namespace surfaceflingerextension {

class QtiWorkDurationsExtension {
public:
    QtiWorkDurationsExtension();
    QtiWorkDurationsExtension(scheduler::VsyncConfiguration* vsyncConfig);
    ~QtiWorkDurationsExtension() = default;

    void qtiUpdateSfOffsets(unordered_map<float, int64_t>* advancedSfOffsets);
    void qtiUpdateWorkDurations(unordered_map<float, pair<int64_t, int64_t>>* workDurationConfigs);

private:
    scheduler::impl::WorkDuration* mQtiVsyncConfiguration = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
