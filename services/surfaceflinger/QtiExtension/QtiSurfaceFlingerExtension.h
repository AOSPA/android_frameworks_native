/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#ifndef QTI_DISPLAY_EXTENSION
#define QTI_DISPLAY_EXTENSION
#endif

#include "QtiSurfaceFlingerExtensionIntf.h"

#include <aidl/vendor/qti/hardware/display/config/BnDisplayConfigCallback.h>
#include <binder/IBinder.h>
#include <composer_extn_intf.h>
#include <list>
#include <map>

#include "../DisplayHardware/HWComposer.h"
#include "../DisplayHardware/PowerAdvisor.h"
#include "../SurfaceFlinger.h"
#include "QtiFeatureManager.h"
#include "QtiHWComposerExtensionIntf.h"
#include "QtiNullExtension.h"
#include "QtiPhaseOffsetsExtension.h"
#include "QtiPowerAdvisorExtension.h"
#include "QtiWorkDurationsExtension.h"
#include "QtiDolphinWrapper.h"
#include "TransactionState.h"
#include "layer_extn_intf.h"

namespace composer {
class ComposerExtnIntf;
class ComposerExtnLib;
class FrameSchedulerIntf;
class DisplayExtnIntf;
class LayerExtnIntf;
} // namespace composer

namespace smomo {
class SmomoIntf;
} // namespace smomo

using aidl::vendor::qti::hardware::display::config::Attributes;
using aidl::vendor::qti::hardware::display::config::BnDisplayConfigCallback;
using aidl::vendor::qti::hardware::display::config::CameraSmoothOp;
using aidl::vendor::qti::hardware::display::config::Concurrency;
using aidl::vendor::qti::hardware::display::config::DisplayType;
using aidl::vendor::qti::hardware::display::config::TUIEventType;

using composer::LayerExtnIntf;
using smomo::SmomoIntf;

namespace android::surfaceflingerextension {

class QtiSurfaceFlingerExtension;

/*
 * LayerExtWrapper class
 */
class LayerExtWrapper {
public:
    LayerExtWrapper() {}
    ~LayerExtWrapper();

    bool init();

    LayerExtnIntf* operator->() { return mInst; }
    operator bool() { return mInst != nullptr; }

    LayerExtWrapper(const LayerExtWrapper&) = delete;
    LayerExtWrapper& operator=(const LayerExtWrapper&) = delete;

private:
    LayerExtnIntf* mInst = nullptr;
    void* mLayerExtLibHandle = nullptr;

    using CreateLayerExtnFuncPtr = std::add_pointer<bool(uint16_t, LayerExtnIntf**)>::type;
    using DestroyLayerExtnFuncPtr = std::add_pointer<void(LayerExtnIntf*)>::type;
    CreateLayerExtnFuncPtr mLayerExtCreateFunc;
    DestroyLayerExtnFuncPtr mLayerExtDestroyFunc;
};

/*
 * IDisplayConfig AIDL Callback handler
 */
class DisplayConfigAidlCallbackHandler : public BnDisplayConfigCallback {
public:
    DisplayConfigAidlCallbackHandler(
            android::surfaceflingerextension::QtiSurfaceFlingerExtensionIntf* sfext);

    virtual ndk::ScopedAStatus notifyCameraSmoothInfo(CameraSmoothOp op, int fps) override;
    virtual ndk::ScopedAStatus notifyCWBBufferDone(
            int32_t in_error,
            const ::aidl::android::hardware::common::NativeHandle& in_buffer) override;
    virtual ndk::ScopedAStatus notifyQsyncChange(bool in_qsyncEnabled, int32_t in_refreshRate,
                                                 int32_t in_qsyncRefreshRate) override;
    virtual ndk::ScopedAStatus notifyIdleStatus(bool in_isIdle) override;
    virtual ndk::ScopedAStatus notifyResolutionChange(int32_t displayId,
                                                      const Attributes& attr) override;

    virtual ndk::ScopedAStatus notifyFpsMitigation(int32_t displayId, const Attributes& attr,
                                                   Concurrency concurrency) override;
    virtual ndk::ScopedAStatus notifyTUIEventDone(int32_t in_error, DisplayType in_disp_type,
                                                  TUIEventType in_eventType) override;

private:
    android::surfaceflingerextension::QtiSurfaceFlingerExtensionIntf* mQtiSFExtnIntf;
};

/*
 * QtiSurfaceFlingerExtension class
 */
class QtiSurfaceFlingerExtension : public QtiSurfaceFlingerExtensionIntf {
public:
    QtiSurfaceFlingerExtension();
    ~QtiSurfaceFlingerExtension();

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
    bool qtiIsInternalPresentationDisplays() { return mQtiInternalPresentationDisplays; };
    bool qtiIsWakeUpPresentationDisplays() { return mQtiWakeUpPresentationDisplays; };
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
    composer::DisplayExtnIntf* qtiGetDisplayExtn() { return mQtiDisplayExtnIntf; }
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
    status_t qtiSetDisplayElapseTime(std::optional<std::chrono::steady_clock::time_point>
                                             earliestPresentTime) const override;

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
    void qtiSetLayerAsMask(uint32_t hwcDisplayId, uint64_t layerId) override;


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
    bool qtiIsScreenshot(const std::string& layer_name);

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
     * Methods for Dolphin APIs
     */
    void qtiDolphinSetVsyncPeriod(nsecs_t vsyncPeriod);
    void qtiDolphinTrackBufferIncrement(const char *name);
    void qtiDolphinTrackBufferDecrement(const char *name, int count);
    void qtiDolphinTrackVsyncSignal();

    /*
     * Methods for speculative fence
     */
    void qtiStartUnifiedDraw() override;
    void qtiTryDrawMethod(sp<DisplayDevice> display) override;
    void qtiEndUnifiedDraw(uint32_t hwcDisplayId);

    std::optional<PhysicalDisplayId> qtiGetInternalDisplayId();
    void qtiSetDesiredModeByThermalLevel(float newLevelFps);
    bool qtiIsFpsDeferNeeded(float newFpsRequest) override;
    DisplayModePtr qtiGetModeFromFps(float fps);
    void qtiHandleNewLevelFps(float currFps, float newLevelFps, float* fpsToSet);
    void qtiNotifyResolutionSwitch(int displayId, int32_t width, int32_t height,
                                   int32_t vsyncPeriod) override;
    void qtiSetFrameBufferSizeForScaling(sp<DisplayDevice> displayDevice,
                                         DisplayDeviceState& currentState,
                                         const DisplayDeviceState& drawingState) override;
    void qtiFbScalingOnBoot() override;
    bool qtiFbScalingOnDisplayChange(const wp<IBinder>& displayToken, sp<DisplayDevice> display,
                                     const DisplayDeviceState& drawingState) override;
    void qtiFbScalingOnPowerChange(sp<DisplayDevice> display) override;
    void qtiAllowIdleFallback();

private:
    SmomoIntf* qtiGetSmomoInstance(const uint32_t layerStackId) const;
    bool qtiIsInternalDisplay(const sp<DisplayDevice>& display);
    void qtiSetupDisplayExtnFeatures();

    SurfaceFlinger* mQtiFlinger = nullptr;
    composer::ComposerExtnIntf* mQtiComposerExtnIntf = nullptr;
    composer::DisplayExtnIntf* mQtiDisplayExtnIntf = nullptr;
    composer::FrameSchedulerIntf* mQtiFrameSchedulerExtnIntf = nullptr;
    QtiFeatureManager* mQtiFeatureManager = nullptr;
    QtiHWComposerExtensionIntf* mQtiHWComposerExtnIntf = nullptr;
    QtiPowerAdvisorExtension* mQtiPowerAdvisorExtn = nullptr;
    QtiPhaseOffsetsExtension* mQtiPhaseOffsetsExtn = nullptr;
    QtiWorkDurationsExtension* mQtiWorkDurationsExtn = nullptr;
    QtiDolphinWrapper* mQtiDolphinWrapper = nullptr;

    bool mQtiSmomoOptimalRefreshActive = false;
    bool mQtiEnabledIDC = false;
    bool mQtiInitVsyncConfigurationExtn = false;
    bool mQtiInternalPresentationDisplays = false;
    bool mQtiSendEarlyWakeUp = false;
    bool mQtiSentInitialFps = false;
    bool mQtiSFExtnBootComplete = false;
    bool mQtiTidSentSuccessfully = false;
    bool mQtiWakeUpPresentationDisplays = false;
    bool mQtiDisplaySizeChanged = false;
    int mQtiFirstApiLevel = 0;
    int mQtiRETid = 0;
    int mQtiSFTid = 0;
    int mQtiUiLayerFrameCount = 180;
    bool mComposerRefreshNotified = false;
    uint32_t mQtiCurrentFps = 0;
    float mQtiThermalLevelFps = 0;
    float mQtiLastCachedFps = 0;
    bool mQtiAllowThermalFpsChange = false;
    bool mQtiHasScreenshot = false;

    std::shared_ptr<IDisplayConfig> mQtiDisplayConfigAidl = nullptr;
    std::shared_ptr<DisplayConfigAidlCallbackHandler> mQtiAidlCallbackHandler = nullptr;
    int64_t mQtiCallbackClientId = -1;
    ::DisplayConfig::ClientInterface* mQtiDisplayConfigHidl = nullptr;

    static bool mQtiSDirectStreaming;

    std::list<sp<DisplayDevice>> mQtiDisplaysList = {};
    std::mutex mQtiEarlyWakeUpMutex;
    std::mutex mQtiSetDisplayElapseTimeMutex;
    std::unordered_map<float, int64_t> mQtiAdvancedSfOffsets;
    std::unordered_map<float, std::pair<int64_t, int64_t>> mQtiWorkDurationConfigsMap;

    LayerExtWrapper mQtiLayerExt;

    struct SmomoInfo {
        uint64_t displayId;
        uint32_t layerStackId;
        bool active = false;
        SmomoIntf* smoMo = nullptr;
    };

    struct VisibleLayerInfo {
        std::vector<std::string> layerName;
        std::vector<int32_t> layerSequence;
    };
    std::unordered_map<DisplayId, VisibleLayerInfo> mQtiVisibleLayerInfoMap;

    std::vector<SmomoInfo> mQtiSmomoInstances{};
};

} // namespace android::surfaceflingerextension
