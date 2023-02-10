/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../SurfaceFlinger.h"
#include "QtiSurfaceFlingerExtensionIntf.h"

namespace android {

namespace surfaceflingerextension {

class QtiNullExtension : public QtiSurfaceFlingerExtensionIntf {
public:
    QtiNullExtension();
    QtiNullExtension(SurfaceFlinger* flinger) : mQtiFlinger(flinger) {}
    ~QtiNullExtension() = default;

    void qtiInit(SurfaceFlinger* flinger) override;
    QtiSurfaceFlingerExtensionIntf* qtiPostInit(android::impl::HWComposer& hwc,
                                                Hwc2::impl::PowerAdvisor* powerAdvisor,
                                                scheduler::VsyncConfiguration* vsyncConfig,
                                                Hwc2::Composer* composerHal) override;
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
    void qtiUpdateOnComposerHalHotplug(hal::HWDisplayId hwcDisplayId, hal::Connection connection,
                                       std::optional<DisplayIdentificationInfo> info) override;
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
    status_t qtiBinderSetPowerMode(uint64_t displayId, int32_t mode, int32_t tile_h_loc,
                                   int32_t tile_v_loc) override;
    status_t qtiBinderSetPanelBrightnessTiled(uint64_t displayId, int32_t level, int32_t tile_h_loc,
                                              int32_t tile_v_loc) override;
    status_t qtiBinderSetWideModePreference(uint64_t displayId, int32_t pref) override;
    void qtiSetPowerMode(const sp<IBinder>& displayToken, int mode) override;
    void qtiSetPowerModeOverrideConfig(sp<DisplayDevice> display) override;

    /*
     * Methods for Virtual, WiFi, and Secure Displays
     */
    VirtualDisplayId qtiAcquireVirtualDisplay(ui::Size, ui::PixelFormat,
                                              bool canAllocateHwcForVDS) override;
    bool qtiCanAllocateHwcDisplayIdForVDS(const DisplayDeviceState& state) override;
    bool qtiCanAllocateHwcDisplayIdForVDS(uint64_t usage) override;
    void qtiCheckVirtualDisplayHint(const Vector<DisplayState>& displays) override;
    void qtiCreateVirtualDisplay(int width, int height, int format) override;
    void qtiHasProtectedLayer(bool* hasProtectedLayer) override;
    bool qtiIsSecureDisplay(sp<const GraphicBuffer> buffer) override;
    bool qtiIsSecureCamera(sp<const GraphicBuffer> buffer) override;

private:
    SurfaceFlinger* mQtiFlinger = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
