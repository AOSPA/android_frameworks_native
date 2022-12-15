/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "QtiSurfaceFlingerExtensionIntf.h"

namespace android {

namespace surfaceflingerextension {

class QtiNullExtension : public QtiSurfaceFlingerExtensionIntf {
public:
    QtiNullExtension();
    ~QtiNullExtension() = default;

    void qtiInit(SurfaceFlinger* flinger) override;
    QtiSurfaceFlingerExtensionIntf* qtiPostInit(
            android::impl::HWComposer& hwc, Hwc2::impl::PowerAdvisor* powerAdvisor,
            scheduler::VsyncConfiguration* vsyncConfig) override;
    void qtiSetVsyncConfiguration(scheduler::VsyncConfiguration* vsyncConfig) override;
    void qtiSetTid() override;
    bool qtiGetHwcDisplayId(const sp<DisplayDevice>& display, uint32_t* hwcDisplayId) override;
    void qtiHandlePresentationDisplaysEarlyWakeup(size_t updatingDisplays,
                                                  uint32_t layerStackId) override;
    bool qtiIsInternalPresentationDisplays() override;
    bool qtiIsWakeUpPresentationDisplays() override;
    void qtiResetEarlyWakeUp() override;
    void qtiSetDisplayExtnActiveConfig(uint32_t displayId, uint32_t activeConfigId) override;
    void qtiUpdateDisplayExtension(uint32_t displayId, uint32_t configId, bool connected) override;
    void qtiUpdateDisplaysList(sp<DisplayDevice> display, bool addDisplay) override;
    void qtiUpdateOnProcessDisplayHotplug(uint32_t hwcDisplayId, hal::Connection connection,
                                          PhysicalDisplayId id) override;
    void qtiUpdateOnComposerHalHotplug(hal::HWDisplayId hwcDisplayId,
                                       hal::Connection connection) override;
    void qtiUpdateInternalDisplaysPresentationMode() override;

    /*
     * Methods that call the FeatureManager APIs.
     */
    bool qtiIsExtensionFeatureEnabled(QtiFeature feature) override;

    /*
     * Methods used by SurfaceFlinger DisplayHardware.
     */
    status_t qtiSetDisplayElapseTime(
            std::chrono::steady_clock::time_point earliestPresentTime) const override;

    /*
     * Methods that call the DisplayExtension APIs.
     */
    void qtiSendCompositorTid() override;
    void qtiSendInitialFps(uint32_t fps) override;
    void qtiNotifyDisplayUpdateImminent() override;
    void qtiSetContentFps(uint32_t contentFps) override;
    void qtiSetEarlyWakeUpConfig(const sp<DisplayDevice>& display, hal::PowerMode mode) override;
    void qtiUpdateVsyncConfiguration() override;

    /*
     * Methods that call FrameScheduler APIs.
     */
    void qtiUpdateFrameScheduler() override;

    /*
     * Methods that call the IDisplayConfig APIs.
     */
    status_t qtiGetDebugProperty(string prop, string* value) override;
    status_t qtiIsSupportedConfigSwitch(const sp<IBinder>& displayToken, int config) override;
};

} // namespace surfaceflingerextension
} // namespace android
