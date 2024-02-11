/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "QtiNullExtension.h"
#include <ui/DisplayId.h>

#include "MutexUtils.h"

namespace android::surfaceflingerextension {

QtiNullExtension::QtiNullExtension() {
    ALOGI("QtiNullExtension enabled");
}

void QtiNullExtension::qtiInit(SurfaceFlinger* flinger) {}
QtiSurfaceFlingerExtensionIntf* QtiNullExtension::qtiPostInit(
        android::impl::HWComposer& hwc, Hwc2::impl::PowerAdvisor* powerAdvisor,
        scheduler::VsyncConfiguration* vsyncConfig, Hwc2::Composer* composerHal) {
    return this;
}
void QtiNullExtension::qtiSetVsyncConfiguration(scheduler::VsyncConfiguration* vsyncConfig) {}
void QtiNullExtension::qtiSetTid() {}
bool QtiNullExtension::qtiGetHwcDisplayId(const sp<DisplayDevice>& display,
                                          uint32_t* hwcDisplayId) {
    return false;
}
void QtiNullExtension::qtiHandlePresentationDisplaysEarlyWakeup(size_t updatingDisplays,
                                                                uint32_t layerStackId) {}

bool QtiNullExtension::qtiIsInternalPresentationDisplays() {
    return false;
}
bool QtiNullExtension::qtiIsWakeUpPresentationDisplays() {
    return false;
}
void QtiNullExtension::qtiResetEarlyWakeUp() {}
void QtiNullExtension::qtiSetDisplayExtnActiveConfig(uint32_t displayId, uint32_t activeConfigId) {}
void QtiNullExtension::qtiUpdateDisplayExtension(uint32_t displayId, uint32_t configId,
                                                 bool connected) {}
void QtiNullExtension::qtiUpdateDisplaysList(sp<DisplayDevice> display, bool addDisplay) {}
void QtiNullExtension::qtiUpdateOnProcessDisplayHotplug(uint32_t hwcDisplayId,
                                                        hal::Connection connection,
                                                        PhysicalDisplayId id) {}
void QtiNullExtension::qtiUpdateOnComposerHalHotplug(
        hal::HWDisplayId hwcDisplayId, hal::Connection connection,
        std::optional<DisplayIdentificationInfo> info) {}
void QtiNullExtension::qtiUpdateInternalDisplaysPresentationMode() {}

QtiHWComposerExtensionIntf* QtiNullExtension::qtiGetHWComposerExtensionIntf() {
    return nullptr;
}

bool QtiNullExtension::qtiLatchMediaContent(sp<Layer> layer) {
    return false;
}
void QtiNullExtension::qtiUpdateBufferData(bool qtiLatchMediaContent, const layer_state_t& s) {}

void QtiNullExtension::qtiOnComposerHalRefresh() {}

/*
 * Methods that call the FeatureManager APIs.
 */
bool QtiNullExtension::qtiIsExtensionFeatureEnabled(QtiFeature feature) {
    return false;
}

/*
 * Methods used by SurfaceFlinger DisplayHardware.
 */
status_t QtiNullExtension::qtiSetDisplayElapseTime(
        std::optional<std::chrono::steady_clock::time_point> earliestPresentTime) const {
    return OK;
}

/*
 * Methods that call the DisplayExtension APIs.
 */
void QtiNullExtension::qtiSendCompositorTid() {}
void QtiNullExtension::qtiSendInitialFps(uint32_t fps) {}
void QtiNullExtension::qtiNotifyDisplayUpdateImminent() {}
void QtiNullExtension::qtiSetContentFps(uint32_t contentFps) {}
void QtiNullExtension::qtiSetEarlyWakeUpConfig(const sp<DisplayDevice>& display,
                                               hal::PowerMode mode, bool isInternal) {}
void QtiNullExtension::qtiUpdateVsyncConfiguration() {}

/*
 * Methods that call FrameScheduler APIs.
 */
//void QtiNullExtension::qtiUpdateFrameScheduler() {}

/*
 * Methods that call the IDisplayConfig APIs.
 */
status_t QtiNullExtension::qtiGetDebugProperty(string prop, string* value) {
    return OK;
}
status_t QtiNullExtension::qtiIsSupportedConfigSwitch(const sp<IBinder>& displayToken, int config) {
    return OK;
}
status_t QtiNullExtension::qtiBinderSetPowerMode(uint64_t displayId, int32_t mode,
                                                 int32_t tile_h_loc, int32_t tile_v_loc) {
    return OK;
}
status_t QtiNullExtension::qtiBinderSetPanelBrightnessTiled(uint64_t displayId, int32_t level,
                                                            int32_t tile_h_loc,
                                                            int32_t tile_v_loc) {
    return OK;
}
status_t QtiNullExtension::qtiBinderSetWideModePreference(uint64_t displayId, int32_t pref) {
    return OK;
}
status_t QtiNullExtension::qtiDoDumpContinuous(int fd, const DumpArgs& args) {
    return OK;
}
void QtiNullExtension::qtiDumpDrawCycle(bool prePrepare) {}

/*
 * Methods for Virtual, WiFi, and Secure Displays
 */

std::optional<android::VirtualDisplayId> QtiNullExtension::qtiAcquireVirtualDisplay(
        ui::Size resolution, ui::PixelFormat format, bool canAllocateHwcForVDS) {
    ConditionalLock lock(mQtiFlinger->mStateLock,
                         std::this_thread::get_id() != mQtiFlinger->mMainThreadId);
    return mQtiFlinger->acquireVirtualDisplay(resolution, format);
}
bool QtiNullExtension::qtiCanAllocateHwcDisplayIdForVDS(const DisplayDeviceState& state) {
    return false;
}
bool QtiNullExtension::qtiCanAllocateHwcDisplayIdForVDS(uint64_t usage) {
    return false;
}
void QtiNullExtension::qtiCheckVirtualDisplayHint(const Vector<DisplayState>& displays) {}
void QtiNullExtension::qtiCreateVirtualDisplay(int width, int height, int format) {}
void QtiNullExtension::qtiHasProtectedLayer(bool* hasProtectedLayer) {}
bool QtiNullExtension::qtiIsSecureDisplay(sp<const GraphicBuffer> buffer) {
    return false;
}
bool QtiNullExtension::qtiIsSecureCamera(sp<const GraphicBuffer> buffer) {
    return false;
}

void QtiNullExtension::qtiSetPowerMode(const sp<IBinder>& displayToken, int mode) {
    mQtiFlinger->setPowerMode(displayToken, mode);
}
void QtiNullExtension::qtiSetPowerModeOverrideConfig(sp<DisplayDevice> display) {}

/*
 * Methods for SmoMo Interface
 */
void QtiNullExtension::qtiCreateSmomoInstance(const DisplayDeviceState& state) {}
void QtiNullExtension::qtiDestroySmomoInstance(const sp<DisplayDevice>& display) {}
void QtiNullExtension::qtiSetRefreshRates(PhysicalDisplayId displayId) {}
void QtiNullExtension::qtiSetRefreshRateTo(int32_t refreshRate) {}
//void QtiNullExtension::qtiSyncToDisplayHardware() {}
void QtiNullExtension::qtiUpdateSmomoState() {}
void QtiNullExtension::qtiSetDisplayAnimating() {}
void QtiNullExtension::qtiUpdateSmomoLayerInfo(
        sp<Layer> layer, int64_t desiredPresentTime, bool isAutoTimestamp,
        std::shared_ptr<renderengine::ExternalTexture> buffer, BufferData& bufferData) {}
void QtiNullExtension::qtiScheduleCompositeImmed() {}
void QtiNullExtension::qtiSetPresentTime(uint32_t layerStackId, int sequence,
                                         nsecs_t desiredPresentTime) {}
void QtiNullExtension::qtiOnVsync(nsecs_t expectedVsyncTime) {}
bool QtiNullExtension::qtiIsFrameEarly(uint32_t layerStackId, int sequence,
                                       nsecs_t desiredPresentTime) {
    return false;
}
void QtiNullExtension::qtiUpdateLayerState(int numLayers) {}
void QtiNullExtension::qtiUpdateSmomoLayerStackId(hal::HWDisplayId hwcDisplayId,
                                                  uint32_t curLayerStackId,
                                                  uint32_t drawLayerStackId) {}
uint32_t QtiNullExtension::qtiGetLayerClass(std::string mName) {
    return 0;
}
void QtiNullExtension::qtiSetVisibleLayerInfo(DisplayId displayId,
                                                  const char* name, int32_t sequence) {}
bool QtiNullExtension::qtiIsSmomoOptimalRefreshActive() {
    return false;
}

/*
 * Methods for speculative fence
 */
void QtiNullExtension::qtiStartUnifiedDraw() {}
void QtiNullExtension::qtiTryDrawMethod(sp<DisplayDevice> display) {}

/*
 * Methods for Dolphin Interface
 */
void QtiNullExtension::qtiDolphinSetVsyncPeriod(nsecs_t vsyncPeriod) {}
void QtiNullExtension::qtiDolphinTrackBufferIncrement(const char *name) {}
void QtiNullExtension::qtiDolphinTrackBufferDecrement(const char *name, int count) {}
void QtiNullExtension::qtiDolphinTrackVsyncSignal() {}

bool QtiNullExtension::qtiIsFpsDeferNeeded(float newFpsRequest) {
    return false;
}
void QtiNullExtension::qtiNotifyResolutionSwitch(int displayId, int32_t width, int32_t height,
                                                 int32_t vsyncPeriod) {}
void QtiNullExtension::qtiSetFrameBufferSizeForScaling(sp<DisplayDevice> displayDevice,
                                                       DisplayDeviceState& currentState,
                                                       const DisplayDeviceState& drawingState) {}
void QtiNullExtension::qtiFbScalingOnBoot() {}
bool QtiNullExtension::qtiFbScalingOnDisplayChange(const wp<IBinder>& displayToken,
                                                   sp<DisplayDevice> display,
                                                   const DisplayDeviceState& drawingState) {
    return false;
}

void QtiNullExtension::qtiFbScalingOnPowerChange(sp<DisplayDevice> display) {}
void QtiNullExtension::qtiDumpMini(std::string& result) {}

} // namespace android::surfaceflingerextension
