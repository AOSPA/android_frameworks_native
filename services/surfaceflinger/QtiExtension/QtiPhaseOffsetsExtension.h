/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../Scheduler/VsyncConfiguration.h"

#include <unordered_map>

using std::pair;
using std::unordered_map;

namespace android::scheduler {
class VsyncConfiguration;
}

namespace android::scheduler::impl {
class VsyncConfiguration;
}

namespace android {

namespace surfaceflingerextension {

class QtiPhaseOffsetsExtension {
public:
    QtiPhaseOffsetsExtension() {}
    QtiPhaseOffsetsExtension(scheduler::VsyncConfiguration* vsyncConfig);
    ~QtiPhaseOffsetsExtension() = default;

    void qtiUpdateSfOffsets(unordered_map<float, int64_t>* advancedSfOffsets);
    void qtiUpdateWorkDurations(unordered_map<float, pair<int64_t, int64_t>>* workDurationConfigs);

private:
    scheduler::impl::VsyncConfiguration* mQtiVsyncConfiguration = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
