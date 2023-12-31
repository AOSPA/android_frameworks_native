/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <SurfaceFlinger.h>
#include "QtiSurfaceFlingerExtensionIntf.h"

namespace android::surfaceflingerextension {

class QtiNullExtension : public QtiSurfaceFlingerExtensionIntf {
public:
    QtiNullExtension();
    QtiNullExtension(SurfaceFlinger* flinger) : mQtiFlinger(flinger) {}
    ~QtiNullExtension() = default;

    void qtiInit(SurfaceFlinger* flinger) override;
    QtiSurfaceFlingerExtensionIntf* qtiPostInit(android::impl::HWComposer& hwc,
                                                Hwc2::impl::PowerAdvisor* powerAdvisor,
                                                VsyncConfiguration* vsyncConfig,
                                                Hwc2::Composer* composerHal) override;
    void qtiSetVsyncConfiguration(VsyncConfiguration* vsyncConfig) override;
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
    QtiHWComposerExtensionIntf* qtiGetHWComposerExtensionIntf() override;
    composer::DisplayExtnIntf* qtiGetDisplayExtn() { return nullptr; }
    bool qtiLatchMediaContent(sp<Layer> layer) override;
    void qtiUpdateBufferData(bool qtiLatchMediaContent, const layer_state_t& s) override;
    void qtiOnComposerHalRefresh() override;

    /*
     * Methods that call the FeatureManager APIs.
     */
    bool qtiIsExtensionFeatureEnabled(QtiFeature feature) override;

    /*
     * Methods used by SurfaceFlinger DisplayHardware.
     */
    status_t qtiSetDisplayElapseTime(
            std::optional<std::chrono::steady_clock::time_point> earliestPresentTime) const override;

    /*
     * Methods that call the DisplayExtension APIs.
     */
    void qtiSendCompositorTid() override;
    void qtiSendInitialFps(uint32_t fps) override;
    void qtiNotifyDisplayUpdateImminent() override;
    void qtiSetContentFps(uint32_t contentFps) override;
    void qtiSetEarlyWakeUpConfig(const sp<DisplayDevice>& display, hal::PowerMode mode,
                                 bool isInternal) override;
    void qtiUpdateVsyncConfiguration() override;

    /*
     * Methods that call FrameScheduler APIs.
     */
    //void qtiUpdateFrameScheduler() override;

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
    void qtiSetLayerAsMask(uint32_t hwcDisplayId __unused, uint64_t layerId __unused) override{};

    /*
     * Methods for Virtual, WiFi, and Secure Displays
     */
    std::optional<VirtualDisplayId> qtiAcquireVirtualDisplay(ui::Size, ui::PixelFormat,
                                                             bool canAllocateHwcForVDS) override;
    bool qtiCanAllocateHwcDisplayIdForVDS(const DisplayDeviceState& state) override;
    bool qtiCanAllocateHwcDisplayIdForVDS(uint64_t usage) override;
    void qtiCheckVirtualDisplayHint(const Vector<DisplayState>& displays) override;
    void qtiCreateVirtualDisplay(int width, int height, int format) override;
    void qtiHasProtectedLayer(bool* hasProtectedLayer) override;
    bool qtiIsSecureDisplay(sp<const GraphicBuffer> buffer) override;
    bool qtiIsSecureCamera(sp<const GraphicBuffer> buffer) override;

    /*
     * Methods for SmoMo Interface
     */
    void qtiCreateSmomoInstance(const DisplayDeviceState& state) override;
    void qtiDestroySmomoInstance(const sp<DisplayDevice>& display) override;
    void qtiSetRefreshRates(PhysicalDisplayId displayId) override;
    void qtiSetRefreshRateTo(int32_t refreshRate) override;
    //void qtiSyncToDisplayHardware() override;
    void qtiUpdateSmomoState() override;
    void qtiSetDisplayAnimating() override;
    void qtiUpdateSmomoLayerInfo(sp<Layer> layer, int64_t desiredPresentTime, bool isAutoTimestamp,
                                 std::shared_ptr<renderengine::ExternalTexture> buffer,
                                 BufferData& bufferData) override;
    void qtiScheduleCompositeImmed() override;
    void qtiSetPresentTime(uint32_t layerStackId, int sequence,
                           nsecs_t desiredPresentTime) override;
    void qtiOnVsync(nsecs_t expectedVsyncTime) override;
    bool qtiIsFrameEarly(uint32_t layerStackId, int sequence, nsecs_t desiredPresentTime) override;
    void qtiUpdateLayerState(int numLayers) override;
    void qtiUpdateSmomoLayerStackId(hal::HWDisplayId hwcDisplayId, uint32_t curLayerStackId,
                                    uint32_t drawLayerStackId) override;
    uint32_t qtiGetLayerClass(std::string mName) override;
    void qtiSetVisibleLayerInfo(DisplayId displayId,
                                    const char* name, int32_t sequence) override;
    bool qtiIsSmomoOptimalRefreshActive() override;

    /*
     * Methods for speculative fence
     */
    void qtiStartUnifiedDraw() override;
    void qtiTryDrawMethod(sp<DisplayDevice> display) override;

    /*
     * Methods for Dolphin APIs
     */
    void qtiDolphinSetVsyncPeriod(nsecs_t vsyncPeriod);
    void qtiDolphinTrackBufferIncrement(const char *name);
    void qtiDolphinTrackBufferDecrement(const char *name, int count);
    void qtiDolphinTrackVsyncSignal();

    bool qtiIsFpsDeferNeeded(float newFpsRequest) override;
    void qtiNotifyResolutionSwitch(int displayId, int32_t width, int32_t height,
                                   int32_t vsyncPeriod) override;
    void qtiSetFrameBufferSizeForScaling(sp<DisplayDevice> displayDevice,
                                         DisplayDeviceState& currentState,
                                         const DisplayDeviceState& drawingState) override;
    void qtiFbScalingOnBoot() override;
    bool qtiFbScalingOnDisplayChange(const wp<IBinder>& displayToken, sp<DisplayDevice> display,
                                     const DisplayDeviceState& drawingState) override;
    void qtiFbScalingOnPowerChange(sp<DisplayDevice> display) override;

private:
    SurfaceFlinger* mQtiFlinger = nullptr;
};

} // namespace android::surfaceflingerextension
