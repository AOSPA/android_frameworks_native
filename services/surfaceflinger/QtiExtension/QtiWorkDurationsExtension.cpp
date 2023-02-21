/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiPhaseOffsetsExtension.h"

#include <log/log.h>

namespace android::surfaceflingerextension {

namespace {
nsecs_t sfDurationToOffset(std::chrono::nanoseconds sfDuration, nsecs_t vsyncDuration) {
    return vsyncDuration - sfDuration.count();
}

nsecs_t appDurationToOffset(std::chrono::nanoseconds appDuration,
                            std::chrono::nanoseconds sfDuration, nsecs_t vsyncDuration) {
    return vsyncDuration - (appDuration + sfDuration).count() % vsyncDuration;
}
} // namespace

QtiWorkDurationsExtension::QtiWorkDurationsExtension(scheduler::VsyncConfiguration* vsyncConfig) {
    if (!vsyncConfig) {
        ALOGW("Invalid pointer to VsyncConfiguration passed");
    } else {
        mQtiVsyncConfiguration = static_cast<scheduler::impl::WorkDuration*>(vsyncConfig);
        ALOGV_IF(mQtiVsyncConfiguration, "Successfully created Work Durations Extension %p",
                 mQtiVsyncConfiguration);
    }
}

void QtiWorkDurationsExtension::qtiUpdateWorkDurations(
        unordered_map<float, pair<int64_t, int64_t>>* workDurationConfigs) {
    if (!mQtiVsyncConfiguration) {
        return;
    }

    std::lock_guard lock(mQtiVsyncConfiguration->mLock);
    for (auto& item : mQtiVsyncConfiguration->mOffsetsCache) {
        int fps = item.first.getIntValue();
        bool foundWorkDurationsConfig = false;
        auto config = workDurationConfigs->find(fps);
        if (config != workDurationConfigs->end()) {
            foundWorkDurationsConfig = true;
        }

        // Update the config for the specified refresh rates or refresh rates lower than 60fps
        if (foundWorkDurationsConfig || (fps < 60)) {
            auto vsyncDuration = item.first.getPeriodNsecs();
            auto& [early, earlyGpu, late, hwcMinWorkDuration] = item.second;
            auto sfWorkDuration =
                    std::chrono::nanoseconds(static_cast<int64_t>(vsyncDuration * 0.75));
            auto appWorkDuration = (mQtiVsyncConfiguration->mAppDuration == -1)
                    ? std::chrono::nanoseconds(vsyncDuration)
                    : std::chrono::nanoseconds(mQtiVsyncConfiguration->mAppDuration);

            if (foundWorkDurationsConfig) {
                sfWorkDuration = std::chrono::nanoseconds(config->second.first);
                appWorkDuration = (config->second.second == 0)
                        ? appWorkDuration
                        : std::chrono::nanoseconds(config->second.second);
            }

            late.sfWorkDuration = std::chrono::nanoseconds(sfWorkDuration);
            late.appWorkDuration = appWorkDuration;
            late.sfOffset = sfDurationToOffset(late.sfWorkDuration, vsyncDuration);
            late.appOffset =
                    appDurationToOffset(late.appWorkDuration, late.sfWorkDuration, vsyncDuration);

            early.sfWorkDuration = late.sfWorkDuration;
            early.sfOffset = sfDurationToOffset(late.sfWorkDuration, vsyncDuration);
            early.appWorkDuration = late.appWorkDuration;
            early.appOffset = late.appOffset;

            earlyGpu.sfWorkDuration = late.sfWorkDuration;
            earlyGpu.sfOffset = sfDurationToOffset(late.sfWorkDuration, vsyncDuration);
            earlyGpu.appWorkDuration = late.appWorkDuration;
            earlyGpu.appOffset = late.appOffset;
        }
    }
}

void QtiWorkDurationsExtension::qtiUpdateSfOffsets(
        unordered_map<float, int64_t>* advancedSfOffsets) {
    ALOGW("WorkDurations not supported when PhaseOffsets is in use");
}

} // namespace android::surfaceflingerextension
