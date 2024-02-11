/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <string>

#include "../SurfaceFlinger.h"
#include "../Scheduler/VsyncConfiguration.h"

using std::string;

namespace composer {
class DisplayExtnIntf;
} // namespace composer
using android::scheduler::VsyncConfiguration;
namespace android::surfaceflingerextension {
using DumpArgs = Vector<String16>;

class QtiHWComposerExtensionIntf;

enum QtiFeature {
    kAdvanceSfOffset = 0,
    kAsyncVdsCreationSupported,
    kDynamicSfIdle,
    kEarlyWakeUp,
    kFbScaling,
    kHwcForVds,
    kHwcForWfd,
    kLatchMediaContent,
    kLayerExtension,
    kPluggableVsyncPrioritized,
    kQsyncIdle,
    kSpecFence,
    kSmomo,
    kSplitLayerExtension,
    kVsyncSourceReliableOnDoze,
    kWorkDurations,
    kIdleFallback,
};

class QtiSurfaceFlingerExtensionIntf {
public:
    virtual ~QtiSurfaceFlingerExtensionIntf() {}

    virtual void qtiInit(SurfaceFlinger* flinger) = 0;
    virtual QtiSurfaceFlingerExtensionIntf* qtiPostInit(android::impl::HWComposer& hwc,
                                                        Hwc2::impl::PowerAdvisor* powerAdvisor,
                                                        VsyncConfiguration* vsyncConfig,
                                                        Hwc2::Composer* composerHal) = 0;
    virtual void qtiSetVsyncConfiguration(VsyncConfiguration* vsyncConfig) = 0;
    virtual void qtiSetTid() = 0;
    virtual bool qtiGetHwcDisplayId(const sp<DisplayDevice>& display, uint32_t* hwcDisplayId) = 0;
    virtual void qtiHandlePresentationDisplaysEarlyWakeup(size_t updatingDisplays,
                                                          uint32_t layerStackId) = 0;
    virtual bool qtiIsInternalPresentationDisplays() = 0;
    virtual bool qtiIsWakeUpPresentationDisplays() = 0;
    virtual void qtiResetEarlyWakeUp() = 0;
    virtual void qtiSetDisplayExtnActiveConfig(uint32_t displayId, uint32_t activeConfigId) = 0;
    virtual void qtiUpdateDisplayExtension(uint32_t displayId, uint32_t configId,
                                           bool connected) = 0;
    virtual void qtiUpdateDisplaysList(sp<DisplayDevice> display, bool addDisplay) = 0;
    virtual void qtiUpdateOnProcessDisplayHotplug(uint32_t hwcDisplayId, hal::Connection connection,
                                                  PhysicalDisplayId id) = 0;
    virtual void qtiUpdateOnComposerHalHotplug(hal::HWDisplayId hwcDisplayId,
                                               hal::Connection connection,
                                               std::optional<DisplayIdentificationInfo> info) = 0;
    virtual void qtiUpdateInternalDisplaysPresentationMode() = 0;
    virtual QtiHWComposerExtensionIntf* qtiGetHWComposerExtensionIntf() = 0;
    virtual composer::DisplayExtnIntf* qtiGetDisplayExtn() = 0;
    virtual bool qtiLatchMediaContent(sp<Layer> layer) = 0;
    virtual void qtiUpdateBufferData(bool qtiLatchMediaContent, const layer_state_t& s) = 0;
    virtual void qtiOnComposerHalRefresh() = 0;

    /*
     * Methods that call the FeatureManager APIs.
     */
    virtual bool qtiIsExtensionFeatureEnabled(QtiFeature feature) = 0;

    /*
     * Methods used by SurfaceFlinger DisplayHardware.
     */
    virtual status_t qtiSetDisplayElapseTime(
            std::optional<std::chrono::steady_clock::time_point> earliestPresentTime) const = 0;

    /*
     * Methods that call the DisplayExtension APIs.
     */
    virtual void qtiSendCompositorTid() = 0;
    virtual void qtiSendInitialFps(uint32_t fps) = 0;
    virtual void qtiNotifyDisplayUpdateImminent() = 0;
    virtual void qtiSetContentFps(uint32_t contentFps) = 0;
    virtual void qtiSetEarlyWakeUpConfig(const sp<DisplayDevice>& display, hal::PowerMode mode,
                                         bool isInternal) = 0;
    virtual void qtiUpdateVsyncConfiguration() = 0;

    /*
     * Methods that call FrameScheduler APIs.
     */
    //virtual void qtiUpdateFrameScheduler() = 0;

    /*
     * Methods that call the IDisplayConfig APIs.
     */
    virtual status_t qtiGetDebugProperty(string prop, string* value) = 0;
    virtual status_t qtiIsSupportedConfigSwitch(const sp<IBinder>& displayToken, int config) = 0;
    virtual status_t qtiBinderSetPowerMode(uint64_t displayId, int32_t mode, int32_t tile_h_loc,
                                           int32_t tile_v_loc) = 0;
    virtual status_t qtiBinderSetPanelBrightnessTiled(uint64_t displayId, int32_t level,
                                                      int32_t tile_h_loc, int32_t tile_v_loc) = 0;
    virtual status_t qtiBinderSetWideModePreference(uint64_t displayId, int32_t pref) = 0;
    virtual void qtiSetPowerMode(const sp<IBinder>& displayToken, int mode) = 0;
    virtual void qtiSetPowerModeOverrideConfig(sp<DisplayDevice> display) = 0;
    virtual void qtiSetLayerAsMask(uint32_t hwcDisplayId, uint64_t layerId) = 0;

    /*
     * Methods for Virtual, WiFi, and Secure Displays
     */
    virtual std::optional<VirtualDisplayId> qtiAcquireVirtualDisplay(ui::Size, ui::PixelFormat,
                                                                     bool canAllocateHwcForVDS) = 0;
    virtual bool qtiCanAllocateHwcDisplayIdForVDS(const DisplayDeviceState& state) = 0;
    virtual bool qtiCanAllocateHwcDisplayIdForVDS(uint64_t usage) = 0;
    virtual void qtiCheckVirtualDisplayHint(const Vector<DisplayState>& displays) = 0;
    virtual void qtiCreateVirtualDisplay(int width, int height, int format) = 0;
    virtual void qtiHasProtectedLayer(bool* hasProtectedLayer) = 0;
    virtual bool qtiIsSecureDisplay(sp<const GraphicBuffer> buffer) = 0;
    virtual bool qtiIsSecureCamera(sp<const GraphicBuffer> buffer) = 0;

    /*
     * Methods for SmoMo Interface
     */
    virtual void qtiCreateSmomoInstance(const DisplayDeviceState& state) = 0;
    virtual void qtiDestroySmomoInstance(const sp<DisplayDevice>& display) = 0;
    virtual void qtiSetRefreshRates(PhysicalDisplayId displayId) = 0;
    virtual void qtiSetRefreshRateTo(int32_t refreshRate) = 0;
    //virtual void qtiSyncToDisplayHardware() = 0;
    virtual void qtiUpdateSmomoState() = 0;
    virtual void qtiSetDisplayAnimating() = 0;
    virtual void qtiUpdateSmomoLayerInfo(sp<Layer> layer, int64_t desiredPresentTime,
                                         bool isAutoTimestamp,
                                         std::shared_ptr<renderengine::ExternalTexture> buffer,
                                         BufferData& bufferData) = 0;
    virtual void qtiScheduleCompositeImmed() = 0;
    virtual void qtiSetPresentTime(uint32_t layerStackId, int sequence,
                                   nsecs_t desiredPresentTime) = 0;
    virtual void qtiOnVsync(nsecs_t expectedVsyncTime) = 0;
    virtual bool qtiIsFrameEarly(uint32_t layerStackId, int sequence,
                                 nsecs_t desiredPresentTime) = 0;
    virtual void qtiUpdateLayerState(int numLayers) = 0;
    virtual void qtiUpdateSmomoLayerStackId(hal::HWDisplayId hwcDisplayId,
                                 uint32_t curLayerStackId, uint32_t drawLayerStackId) = 0;
    virtual uint32_t qtiGetLayerClass(std::string mName) = 0;
    virtual void qtiSetVisibleLayerInfo(DisplayId displayId,
                                 const char* name, int32_t sequence) = 0;
    virtual bool qtiIsSmomoOptimalRefreshActive() = 0;

    /*
     * Methods for Dolphin APIs
     */
    virtual void qtiDolphinSetVsyncPeriod(nsecs_t vsyncPeriod);
    virtual void qtiDolphinTrackBufferIncrement(const char *name);
    virtual void qtiDolphinTrackBufferDecrement(const char *name, int count);
    virtual void qtiDolphinTrackVsyncSignal();

    /*
     * Methods for speculative fence
     */
    virtual void qtiStartUnifiedDraw() = 0;
    virtual void qtiTryDrawMethod(sp<DisplayDevice> display) = 0;

    virtual bool qtiIsFpsDeferNeeded(float newFpsRequest) = 0;
    virtual void qtiNotifyResolutionSwitch(int displayId, int32_t width, int32_t height,
                                           int32_t vsyncPeriod) = 0;
    virtual void qtiSetFrameBufferSizeForScaling(sp<DisplayDevice> displayDevice,
                                                 DisplayDeviceState& currentState,
                                                 const DisplayDeviceState& drawingState) = 0;
    virtual void qtiFbScalingOnBoot() = 0;
    virtual bool qtiFbScalingOnDisplayChange(const wp<IBinder>& displayToken,
                                             sp<DisplayDevice> display,
                                             const DisplayDeviceState& drawingState) = 0;
    virtual void qtiFbScalingOnPowerChange(sp<DisplayDevice> display) = 0;
    virtual void qtiDumpMini(std::string& result) = 0;
    virtual status_t qtiDoDumpContinuous(int fd, const DumpArgs& args) = 0;
    virtual void qtiDumpDrawCycle(bool prePrepare) = 0;
};

} // namespace android::surfaceflingerextension
