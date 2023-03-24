/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiSurfaceFlingerExtension.h"
#include "FrontEnd/LayerHandle.h"
#include "Layer.h"
#include "MutexUtils.h"
#include "QtiGralloc.h"
#include "vendor/qti/hardware/display/composer/3.1/IQtiComposer.h"

#include <Scheduler/VsyncConfiguration.h>
#include <aidl/vendor/qti/hardware/display/config/IDisplayConfig.h>
#include <aidl/vendor/qti/hardware/display/config/IDisplayConfigCallback.h>
#include <vendor/qti/hardware/display/composer/3.1/IQtiComposerClient.h>

#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <composer_extn_intf.h>
#include <config/client_interface.h>
#include <ftl/non_null.h>

#include <Scheduler/VSyncPredictor.h>
#include <Scheduler/VsyncConfiguration.h>
#include <ui/DisplayStatInfo.h>
#include <vector>

using aidl::vendor::qti::hardware::display::config::IDisplayConfig;
using vendor::qti::hardware::display::composer::V3_1::IQtiComposerClient;

using android::hardware::graphics::common::V1_0::BufferUsage;
using PerfHintType = composer::PerfHintType;
using VsyncConfiguration = android::scheduler::VsyncConfiguration;
using VsyncModulator = android::scheduler::VsyncModulator;
using VsyncTracker = android::scheduler::VsyncTracker;
using DisplayStatInfo = android::DisplayStatInfo;


namespace android::scheduler::impl {

class VsyncConfiguration;
} // namespace android::scheduler::impl

namespace DisplayConfig {
class ClientInterface;
} // namespace DisplayConfig

struct ComposerExtnIntf {
    composer::PhaseOffsetExtnIntf* phaseOffsetExtnIntf = nullptr;
};
struct ComposerExtnIntf g_comp_ext_intf_;

composer::ComposerExtnLib composer::ComposerExtnLib::g_composer_ext_lib_;

namespace android::surfaceflingerextension {

bool QtiSurfaceFlingerExtension::mQtiSDirectStreaming;


QtiSurfaceFlingerExtension::QtiSurfaceFlingerExtension() {}
QtiSurfaceFlingerExtension::~QtiSurfaceFlingerExtension() = default;

void QtiSurfaceFlingerExtension::qtiInit(SurfaceFlinger* flinger) {
    if (!flinger) {
        ALOGW("Invalid SF pointer");
        return;
    }

    mQtiFlinger = flinger;
    mQtiComposerExtnIntf = composer::ComposerExtnLib::GetInstance();
    if (!mQtiComposerExtnIntf) {
        ALOGE("Failed to create composer extension");
    }

    if (mQtiComposerExtnIntf) {
        ALOGI("Successfully created composer extension %p", mQtiComposerExtnIntf);
        int ret = mQtiComposerExtnIntf->CreateFrameScheduler(&mQtiFrameSchedulerExtnIntf);
        if (ret == -1 || !mQtiFrameSchedulerExtnIntf) {
            ALOGW("Failed to create frame scheduler extension");
        } else {
            ALOGI("Successfully created frame scheduler extension %p", mQtiFrameSchedulerExtnIntf);
        }

        ret = mQtiComposerExtnIntf->CreateDisplayExtn(&mQtiDisplayExtnIntf);
        if (ret) {
            ALOGW("Failed to create display extension");
        } else {
            ALOGI("Successfully created display extension %p", mQtiDisplayExtnIntf);
        }
    }

    mQtiFeatureManager = new QtiFeatureManager(this);
    if (!mQtiFeatureManager) {
        ALOGE("Failed to initialize QtiFeatureManager");
    } else {
        mQtiFeatureManager->qtiInit();
        bool mUseLayerExt =
                mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kLayerExtension);
        bool mSplitLayerExt =
                mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kSplitLayerExtension);
        if ((mUseLayerExt || mSplitLayerExt) && mQtiLayerExt.init()) {
            ALOGI("Layer extension is enabled");
        }
    }

    if (mQtiComposerExtnIntf) {
        if (mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kAdvanceSfOffset)) {
            int ret = mQtiComposerExtnIntf->CreatePhaseOffsetExtn(
                    &g_comp_ext_intf_.phaseOffsetExtnIntf);
            if (ret) {
                ALOGE("Failed to create PhaseOffset extension");
            } else {
                ALOGI("Created PhaseOffset extension");
            }
        }
    }

    mQtiFirstApiLevel = android::base::GetIntProperty("ro.product.first_api_level", 0);
}

QtiSurfaceFlingerExtensionIntf* QtiSurfaceFlingerExtension::qtiPostInit(
        android::impl::HWComposer& hwc, Hwc2::impl::PowerAdvisor* powerAdvisor,
        scheduler::VsyncConfiguration* vsyncConfig, Hwc2::Composer* composerHal) {
    ndk::SpAIBinder binder(AServiceManager_checkService(
            "vendor.qti.hardware.display.config.IDisplayConfig/default"));

    if (binder.get() == nullptr) {
        ALOGE("DisplayConfig AIDL is not present");
    } else {
        mQtiDisplayConfigAidl = IDisplayConfig::fromBinder(binder);
        if (mQtiDisplayConfigAidl == nullptr) {
            ALOGE("Failed to retrieve DisplayConfig AIDL binder");
        } else {
            ALOGI("Initialized DisplayConfig AIDL %p successfully", mQtiDisplayConfigAidl.get());
            if (mQtiFeatureManager) mQtiFeatureManager->qtiSetIDisplayConfig(mQtiDisplayConfigAidl);
            mQtiEnabledIDC = true;
        }
    }

    // Initialize IDC HIDL only if AIDL is not present
    if (mQtiDisplayConfigAidl == nullptr) {
        int ret = ::DisplayConfig::ClientInterface::Create("SurfaceFlinger" + std::to_string(0),
                                                           nullptr, &mQtiDisplayConfigHidl);
        if (ret || !mQtiDisplayConfigHidl) {
            ALOGE("DisplayConfig HIDL not present");
            mQtiDisplayConfigHidl = nullptr;
        } else {
            ALOGI("Initialized DisplayConfig HIDL %p successfully", mQtiDisplayConfigHidl);
            if (mQtiFeatureManager) mQtiFeatureManager->qtiSetIDisplayConfig(mQtiDisplayConfigHidl);
            mQtiEnabledIDC = true;
        }
    }

    if (mQtiFeatureManager) {
        mQtiFeatureManager->qtiPostInit();
    }

    // When both IDisplayConfig AIDL and HIDL are not available, behave similar to GSI mode
    if (!mQtiEnabledIDC) {
        ALOGW("DisplayConfig HIDL and AIDL are both unavailable - disabling composer extensions");
        return new QtiNullExtension(mQtiFlinger);
    }

    mQtiHWComposerExtnIntf = qtiCreateHWComposerExtension(hwc, composerHal);
    mQtiPowerAdvisorExtn = new QtiPowerAdvisorExtension(powerAdvisor);

    qtiSetVsyncConfiguration(vsyncConfig);
    qtiSetupDisplayExtnFeatures();
    qtiUpdateVsyncConfiguration();
    mQtiSFExtnBootComplete = true;
    return this;
}

void QtiSurfaceFlingerExtension::qtiSetVsyncConfiguration(
        scheduler::VsyncConfiguration* vsyncConfig) {
    if (mQtiInitVsyncConfigurationExtn) {
        return;
    }

    bool useAdvancedSfOffsets =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kAdvanceSfOffset);

    bool useWorkDurations =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kWorkDurations);

    if (useWorkDurations) {
        mQtiWorkDurationsExtn = new QtiWorkDurationsExtension(vsyncConfig);
    } else if (useAdvancedSfOffsets) {
        mQtiPhaseOffsetsExtn = new QtiPhaseOffsetsExtension(vsyncConfig);
    }

    mQtiInitVsyncConfigurationExtn = true;
}

bool QtiSurfaceFlingerExtension::qtiGetHwcDisplayId(const sp<DisplayDevice>& display,
                                                    uint32_t* hwcDisplayId) {
    if (!display) {
        return false;
    }

    const auto displayId = display->getId();
    if (!displayId.value) {
        return false;
    }

    if (display->isVirtual()) {
        const auto virtualDisplayId = HalVirtualDisplayId::tryCast(displayId);
        if (!virtualDisplayId) {
            return false;
        }
        const auto halDisplayId =
                mQtiHWComposerExtnIntf->qtiFromVirtualDisplayId(*virtualDisplayId);
        if (!halDisplayId) {
            return false;
        }
        *hwcDisplayId = static_cast<uint32_t>(*halDisplayId);
    } else {
        const auto physicalDisplayId = PhysicalDisplayId::tryCast(displayId);
        if (!physicalDisplayId) {
            return false;
        }
        const auto halDisplayId =
                mQtiFlinger->getHwComposer().fromPhysicalDisplayId(*physicalDisplayId);
        if (!halDisplayId) {
            return false;
        }
        *hwcDisplayId = static_cast<uint32_t>(*halDisplayId);
    }
    return true;
}

void QtiSurfaceFlingerExtension::qtiHandlePresentationDisplaysEarlyWakeup(size_t updatingDisplays,
                                                                          uint32_t layerStackId) {
    // Filter-out the updating display(s) for early wake-up in Presentation mode.
    uint32_t hwcDisplayId;
    bool internalDisplay = false;
    bool singleUpdatingDisplay = (updatingDisplays == 1);
    bool earlyWakeUpEnabled =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kEarlyWakeUp);

    if (mQtiDisplayExtnIntf && earlyWakeUpEnabled && mQtiInternalPresentationDisplays) {
        ATRACE_CALL();
        if (singleUpdatingDisplay) {
            Mutex::Autolock lock(mQtiFlinger->mStateLock);
            const sp<DisplayDevice> display =
                    mQtiFlinger->findDisplay([layerStackId](const auto& display) {
                        return display.getLayerStack().id == layerStackId;
                    });
            internalDisplay =
                    qtiIsInternalDisplay(display) && qtiGetHwcDisplayId(display, &hwcDisplayId);
        }

#ifdef EARLY_WAKEUP_FEATURE
        if (!singleUpdatingDisplay) {
            // Notify Display Extn for Early Wakeup of displays
            mQtiDisplayExtnIntf->NotifyEarlyWakeUp(false, true);
        } else if (internalDisplay) {
            // Notify Display Extn for Early Wakeup of given display
            mQtiDisplayExtnIntf->NotifyDisplayEarlyWakeUp(hwcDisplayId);
        }
#endif
    }
    mQtiWakeUpPresentationDisplays = false;
}

void QtiSurfaceFlingerExtension::qtiResetEarlyWakeUp() {
    mQtiSendEarlyWakeUp = false;
}

void QtiSurfaceFlingerExtension::qtiSetDisplayExtnActiveConfig(uint32_t displayId,
                                                               uint32_t activeConfigId) {
    ALOGV("setDisplayExtnActiveConfig: Display:%d, ActiveConfig:%d", displayId, activeConfigId);
#ifdef EARLY_WAKEUP_FEATURE
    if (mQtiDisplayExtnIntf) {
        mQtiDisplayExtnIntf->SetActiveConfig(displayId, activeConfigId);
    }
#endif
}

void QtiSurfaceFlingerExtension::qtiSetTid() {
    mQtiSFTid = gettid();
    std::optional<pid_t> re = mQtiFlinger->getRenderEngine().getRenderEngineTid();
    if (re.has_value()) {
        mQtiRETid = *re;
    }

    ALOGI("SF tid %d R tid %d", mQtiSFTid, mQtiRETid);
}

void QtiSurfaceFlingerExtension::qtiUpdateDisplayExtension(uint32_t displayId, uint32_t configId,
                                                           bool connected) {
    ALOGV("UpdateDisplayExtn: Display:%d, Config:%d, Connected:%d", displayId, configId, connected);
    if (!mQtiDisplayExtnIntf) {
        ALOGV("Display extension is not present or ready");
        return;
    }

#ifdef EARLY_WAKEUP_FEATURE
    if (mQtiDisplayExtnIntf) {
        if (connected) {
            mQtiDisplayExtnIntf->RegisterDisplay(displayId);
            mQtiDisplayExtnIntf->SetActiveConfig(displayId, configId);
        } else {
            mQtiDisplayExtnIntf->UnregisterDisplay(displayId);
        }
    }
#endif
}

void QtiSurfaceFlingerExtension::qtiUpdateDisplaysList(sp<DisplayDevice> display, bool addDisplay) {
    if (!display) {
        ALOGW("Attempted to add an invalid display");
        return;
    }

    if (addDisplay) {
        mQtiDisplaysList.push_back(display);
        ALOGV("Added display %s cur_size:%u", to_string(display->getPhysicalId()).c_str(),
              mQtiDisplaysList.size());
    } else {
        auto it = std::find(mQtiDisplaysList.begin(), mQtiDisplaysList.end(), display);
        if (it != mQtiDisplaysList.end()) {
            mQtiDisplaysList.erase(it);
            ALOGV("Removed display %s cur_size:%u", to_string(display->getPhysicalId()).c_str(),
                  mQtiDisplaysList.size());
        }
    }
}

void QtiSurfaceFlingerExtension::qtiUpdateOnProcessDisplayHotplug(uint32_t hwcDisplayId,
                                                                  hal::Connection connection,
                                                                  PhysicalDisplayId id) {
    bool qtiIsInternalDisplay = (mQtiFlinger->getHwComposer().getDisplayConnectionType(id) ==
                                 ui::DisplayConnectionType::Internal);
    bool qtiIsConnected = (connection == hal::Connection::CONNECTED);
    auto qtiActiveConfigId = mQtiFlinger->getHwComposer().getActiveMode(id);

    if (!qtiIsConnected && mQtiInternalPresentationDisplays && qtiIsInternalDisplay) {
        // Update mInternalPresentationDisplays flag
        qtiUpdateInternalDisplaysPresentationMode();
    }

    if (qtiIsInternalDisplay) {
        LOG_ALWAYS_FATAL_IF(!qtiActiveConfigId, "HWC returned no active config");
        qtiUpdateDisplayExtension(hwcDisplayId, *qtiActiveConfigId, qtiIsConnected);
    }

    if (!qtiIsConnected && !qtiIsInternalDisplay) {
        qtiEndUnifiedDraw(hwcDisplayId);
    }
}

void QtiSurfaceFlingerExtension::qtiUpdateOnComposerHalHotplug(
        hal::HWDisplayId hwcDisplayId, hal::Connection connection,
        std::optional<DisplayIdentificationInfo> info) {
    // QTI: Update QTI Extension's displays list when a display is disconnected
    if (connection != hal::Connection::CONNECTED) {
        if (info) {
            qtiUpdateDisplaysList(mQtiFlinger->getDisplayDeviceLocked(info->id), /*add*/ false);
        }
    }
}

void QtiSurfaceFlingerExtension::qtiUpdateInternalDisplaysPresentationMode() {
    mQtiInternalPresentationDisplays = false;
    if (mQtiDisplaysList.size() <= 1) {
        return;
    }

    bool compareStack = false;
    ui::LayerStack previousStackId;
    for (const auto& display : mQtiDisplaysList) {
        if (qtiIsInternalDisplay(display)) {
            auto currentStackId = display->getLayerStack();
            // Compare Layer Stack IDs of Internal Displays
            if (compareStack && (previousStackId != currentStackId)) {
                mQtiInternalPresentationDisplays = true;
                return;
            }
            previousStackId = currentStackId;
            compareStack = true;
        }
    }
}

QtiHWComposerExtensionIntf* QtiSurfaceFlingerExtension::qtiGetHWComposerExtensionIntf() {
    return mQtiHWComposerExtnIntf;
}

bool QtiSurfaceFlingerExtension::qtiLatchMediaContent(sp<Layer> layer) {
    uint64_t usage = layer->getBuffer() ? layer->getBuffer()->getUsage() : 0;
    bool cameraOrVideo = ((usage & GRALLOC_USAGE_HW_CAMERA_WRITE) != 0) ||
            ((usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) != 0);

    return mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kLatchMediaContent) ||
            cameraOrVideo;
}

void QtiSurfaceFlingerExtension::qtiUpdateBufferData(bool qtiLatchMediaContent,
                                                     const layer_state_t& s) {
    if (qtiLatchMediaContent && s.bufferData && s.bufferData->acquireFence &&
        s.bufferData->acquireFence->get() != -1 &&
        (s.bufferData->acquireFence->getStatus() == Fence::Status::Signaled) &&
        (s.bufferData->acquireFence->getSignalTime() == Fence::SIGNAL_TIME_INVALID)) {
        ATRACE_NAME("fence signaled with error. drop");
        s.bufferData->qtiInvalid = true;
    }
}

/*
 * Methods that call the FeatureManager APIs.
 */
bool QtiSurfaceFlingerExtension::qtiIsExtensionFeatureEnabled(QtiFeature feature) {
    return mQtiFeatureManager->qtiIsExtensionFeatureEnabled(feature);
}

/*
 * Methods used by SurfaceFlinger DisplayHardware.
 */
status_t QtiSurfaceFlingerExtension::qtiSetDisplayElapseTime(
        std::chrono::steady_clock::time_point earliestPresentTime) const {
    if (!mQtiFlinger->mBootFinished || !mQtiSFExtnBootComplete || !mQtiHWComposerExtnIntf) {
        return OK;
    }

    nsecs_t sfOffset = mQtiFlinger->mVsyncConfiguration->getCurrentConfigs().late.sfOffset;
    if (!mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kAdvanceSfOffset) ||
        (sfOffset >= 0)) {
        return OK;
    }

    if (mQtiDisplaysList.size() != 1) {
        // Revisit this for multi displays.
        return OK;
    }

    Mutex::Autolock lock(mQtiFlinger->mStateLock);
    for (const auto& [_, display] : mQtiFlinger->mDisplays) {
        if (display->isVirtual()) {
            continue;
        }

        auto timeStamp =
                std::chrono::time_point_cast<std::chrono::nanoseconds>(earliestPresentTime);
        const auto id = HalDisplayId::tryCast(display->getId());
        if (!id) {
            return BAD_VALUE;
        }

        return mQtiHWComposerExtnIntf
                ->qtiSetDisplayElapseTime(*id,
                                          static_cast<uint64_t>(
                                                  timeStamp.time_since_epoch().count()));
    }
    return OK;
}

/*
 *  Methods that call the DisplayExtension APIs.
 */
void QtiSurfaceFlingerExtension::qtiSendCompositorTid() {
#ifdef PASS_COMPOSITOR_TID
    if (!mQtiFlinger->mBootFinished) {
        return;
    }

    if (!mQtiSFTid || !mQtiRETid) {
        qtiSetTid();
    }

    if (!mQtiTidSentSuccessfully && mQtiDisplayExtnIntf) {
        bool sfTid = mQtiDisplayExtnIntf->SendCompositorTid(PerfHintType::kSurfaceFlinger,
                                                            mQtiSFTid) == 0;
        bool reTid =
                mQtiDisplayExtnIntf->SendCompositorTid(PerfHintType::kRenderEngine, mQtiRETid) == 0;

        if (sfTid && reTid) {
            mQtiTidSentSuccessfully = true;
            ALOGV("Successfully sent SF's %d and RE's %d TIDs", mQtiSFTid, mQtiRETid);
        }
    }
#endif
}

void QtiSurfaceFlingerExtension::qtiSendInitialFps(uint32_t fps) {
    if (!mQtiSentInitialFps) {
        qtiSetContentFps(fps);
    }
}

void QtiSurfaceFlingerExtension::qtiNotifyDisplayUpdateImminent() {
    if (!mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kEarlyWakeUp)) {
        mQtiFlinger->mPowerAdvisor->notifyDisplayUpdateImminentAndCpuReset();
        return;
    }

    if (!mQtiPowerAdvisorExtn) {
        return;
    }

#ifdef EARLY_WAKEUP_FEATURE
    bool doEarlyWakeUp = false;
    {
        // Synchronize the critical section.
        std::lock_guard lock(mQtiEarlyWakeUpMutex);
        if (!mQtiSendEarlyWakeUp) {
            mQtiSendEarlyWakeUp = mQtiPowerAdvisorExtn->qtiCanNotifyDisplayUpdateImminent();
            doEarlyWakeUp = mQtiSendEarlyWakeUp;
        }
    }

    if (mQtiDisplayExtnIntf && doEarlyWakeUp) {
        ATRACE_CALL();

        if (mQtiInternalPresentationDisplays) {
            // Notify Display Extn for GPU Early Wakeup only
            mQtiDisplayExtnIntf->NotifyEarlyWakeUp(true, false);
            mQtiWakeUpPresentationDisplays = true;
        } else {
            // Notify Display Extn for GPU and Display Early Wakeup
            mQtiDisplayExtnIntf->NotifyEarlyWakeUp(true, true);
        }
    }
#endif
}

void QtiSurfaceFlingerExtension::qtiSetContentFps(uint32_t contentFps) {
    if (mQtiFlinger->mBootFinished && mQtiDisplayExtnIntf && !mQtiFlinger->mSetActiveModePending &&
        contentFps != mQtiCurrentFps) {
        mQtiSentInitialFps = mQtiDisplayExtnIntf->SetContentFps(contentFps) == 0;

        if (mQtiSentInitialFps) {
            mQtiCurrentFps = contentFps;
            ALOGV("Successfully sent content fps %d", contentFps);
        } else {
            ALOGW("Failed to send content fps %d", contentFps);
        }
    }
}

void QtiSurfaceFlingerExtension::qtiSetEarlyWakeUpConfig(const sp<DisplayDevice>& display,
                                                         hal::PowerMode mode) {
    if (!display) {
        return;
    }

    bool earlyWakeUpEnabled =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kEarlyWakeUp);
    if (earlyWakeUpEnabled && qtiIsInternalDisplay(display)) {
        uint32_t hwcDisplayId;
        if (qtiGetHwcDisplayId(display, &hwcDisplayId)) {
            // Enable/disable Early Wake-up feature on a display based on its Power mode.
            bool enable = (mode == hal::PowerMode::ON) || (mode == hal::PowerMode::DOZE);
            ALOGV("setEarlyWakeUpConfig: Display: %d, Enable: %d", hwcDisplayId, enable);
#ifdef DYNAMIC_EARLY_WAKEUP_CONFIG
            if (mQtiDisplayExtnIntf) {
                mQtiDisplayExtnIntf->SetEarlyWakeUpConfig(hwcDisplayId, enable);
            }
#endif
        }
    }
}

void QtiSurfaceFlingerExtension::qtiUpdateVsyncConfiguration() {
#ifdef PHASE_OFFSET_EXTN
    if (!g_comp_ext_intf_.phaseOffsetExtnIntf || !mQtiFlinger || !mQtiFlinger->mScheduler ||
        !mQtiFlinger->mVsyncConfiguration) {
        return;
    }
    bool useAdvancedSfOffsets =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kAdvanceSfOffset);
    bool useWorkDurations =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kWorkDurations);

    if (useAdvancedSfOffsets && mQtiComposerExtnIntf) {
        if (!mQtiInitVsyncConfigurationExtn) {
            qtiSetVsyncConfiguration(mQtiFlinger->mVsyncConfiguration.get());
        }

        // Populate the fps supported on device in mOffsetCache
        const auto displayOpt = mQtiFlinger->mPhysicalDisplays.get(
                mQtiFlinger->getDefaultDisplayDeviceLocked()->getPhysicalId());

        const auto& display = displayOpt->get();
        const auto& snapshot = display.snapshot();
        const auto& supportedModes = snapshot.displayModes();

        for (const auto& [id, mode] : supportedModes) {
            mQtiFlinger->mVsyncConfiguration->getConfigsForRefreshRate(mode->getFps());
        }

        if (useWorkDurations && mQtiWorkDurationsExtn) {
#ifdef DYNAMIC_APP_DURATIONS
            // Update the Work Durations for the given refresh rates in mOffsets map
            g_comp_ext_intf_.phaseOffsetExtnIntf->GetWorkDurationConfigs(
                    &mQtiWorkDurationConfigsMap);
            mQtiWorkDurationsExtn->qtiUpdateWorkDurations(&mQtiWorkDurationConfigsMap);
#endif
        } else if (mQtiPhaseOffsetsExtn) {
            // Update the Advanced SF Offsets for the given refresh rates in mOffsets map
            g_comp_ext_intf_.phaseOffsetExtnIntf->GetAdvancedSfOffsets(&mQtiAdvancedSfOffsets);
            mQtiPhaseOffsetsExtn->qtiUpdateSfOffsets(&mQtiAdvancedSfOffsets);
        }

        /*
        const auto vsyncConfig = mQtiFlinger->mVsyncModulator->setVsyncConfigSet(
                mQtiFlinger->mVsyncConfiguration->getCurrentConfigs());
        ALOGV("VsyncConfig sfOffset %" PRId64 "\n", vsyncConfig.sfOffset);
        ALOGV("VsyncConfig appOffset %" PRId64 "\n", vsyncConfig.appOffset);
        */
    }
#endif
}
/*
 * Methods that call FrameScheduler APIs.
 */
void QtiSurfaceFlingerExtension::qtiUpdateFrameScheduler() NO_THREAD_SAFETY_ANALYSIS {
    if (mQtiFrameSchedulerExtnIntf == nullptr) {
        return;
    }
    auto &scheduler = mQtiFlinger->mScheduler;
    const sp<Fence>& fence = scheduler->vsyncModulator().getVsyncConfig().sfOffset > 0
            ? mQtiFlinger->mPreviousPresentFences[0].fence
            : mQtiFlinger->mPreviousPresentFences[1].fence;

    if (fence == Fence::NO_FENCE) {
        return;
    }
    int fenceFd = fence->get();
    nsecs_t timeStamp = 0;
    int ret = mQtiFrameSchedulerExtnIntf->UpdateFrameScheduling(fenceFd, &timeStamp);
    if (ret <= 0) {
        return;
    }

    const nsecs_t period = mQtiFlinger->getVsyncPeriodFromHWC();
    scheduler->resyncToHardwareVsync(true, Fps::fromPeriodNsecs(period));
    if (timeStamp > 0) {
        bool periodFlushed = scheduler->addResyncSample(timeStamp, period);
        if (periodFlushed) {
            scheduler->modulateVsync(&VsyncModulator::onRefreshRateChangeCompleted);
        }
    }
}

/*
 * Methods that call the IDisplayConfig APIs.
 */
status_t QtiSurfaceFlingerExtension::qtiGetDebugProperty(string prop, string* value) {
    auto ret = mQtiDisplayConfigAidl->getDebugProperty(prop, value);
    if (ret.isOk()) {
        ALOGV("GetDebugProperty \"%s\" value %s", prop.c_str(), *value->c_str());
    } else {
        ALOGW("Failed to get property %s", prop.c_str());
    }
    return NO_ERROR;
}

status_t QtiSurfaceFlingerExtension::qtiIsSupportedConfigSwitch(const sp<IBinder>& displayToken,
                                                                int config) {
    sp<DisplayDevice> display = nullptr;
    {
        Mutex::Autolock lock(mQtiFlinger->mStateLock);
        display = (mQtiFlinger->getDisplayDeviceLocked(displayToken));
    }

    if (!display) {
        ALOGE("Attempt to switch config %d for invalid display token %p", config,
              displayToken.get());
        return NAME_NOT_FOUND;
    }

    if (mQtiDisplayConfigAidl != nullptr) {
        const auto displayId = PhysicalDisplayId::tryCast(display->getId());
        const auto hwcDisplayId = mQtiFlinger->getHwComposer().fromPhysicalDisplayId(*displayId);
        bool supported = false;
        mQtiDisplayConfigAidl->isSupportedConfigSwitch(static_cast<int>(*hwcDisplayId), config,
                                                       &supported);
        if (!supported) {
            ALOGW("AIDL Switching to config:%d is not supported", config);
            return INVALID_OPERATION;
        } else {
            ALOGI("AIDL Switching to config:%d is supported", config);
            return NO_ERROR;
        }
    }
    if (mQtiDisplayConfigHidl != nullptr) {
        const auto displayId = PhysicalDisplayId::tryCast(display->getId());
        const auto hwcDisplayId = mQtiFlinger->getHwComposer().fromPhysicalDisplayId(*displayId);
        bool supported = false;
        mQtiDisplayConfigHidl->IsSupportedConfigSwitch(static_cast<uint32_t>(*hwcDisplayId),
                                                       static_cast<uint32_t>(config), &supported);
        if (!supported) {
            ALOGW("HIDL Switching to config:%d is not supported", config);
            return INVALID_OPERATION;
        } else {
            ALOGI("HIDL Switching to config:%d is supported", config);
            return NO_ERROR;
        }
    }

    return NO_ERROR;
}

status_t QtiSurfaceFlingerExtension::qtiBinderSetPowerMode(uint64_t displayId, int32_t mode,
                                                           int32_t tile_h_loc, int32_t tile_v_loc) {
    uint32_t num_h_tiles = 1;
    uint32_t num_v_tiles = 1;
    status_t err = NO_ERROR;

    hal::PowerMode power_mode = static_cast<hal::PowerMode>(mode);

    if (tile_h_loc < 0) {
        ALOGI("Debug: Set display = %llu, power mode = %d", (unsigned long long)displayId, mode);
        if (const auto dispId = DisplayId::fromValue<PhysicalDisplayId>(displayId); dispId) {
            qtiSetPowerMode(mQtiFlinger->getPhysicalDisplayToken(dispId.value()), mode);
        }
    } else {
        if (mQtiDisplayConfigHidl) {
            ::DisplayConfig::PowerMode hwcMode = ::DisplayConfig::PowerMode::kOff;
            switch (power_mode) {
                case hal::PowerMode::DOZE:
                    hwcMode = ::DisplayConfig::PowerMode::kDoze;
                    break;
                case hal::PowerMode::ON:
                    hwcMode = ::DisplayConfig::PowerMode::kOn;
                    break;
                case hal::PowerMode::DOZE_SUSPEND:
                    hwcMode = ::DisplayConfig::PowerMode::kDozeSuspend;
                    break;
                default:
                    break;
            }

            // A regular display has one h tile and one v tile.
            mQtiDisplayConfigHidl->GetDisplayTileCount(displayId, &num_h_tiles, &num_v_tiles);
            if (((num_h_tiles * num_v_tiles) < 2) ||
                static_cast<uint32_t>(tile_h_loc) >= num_h_tiles ||
                static_cast<uint32_t>(tile_v_loc) >= num_v_tiles) {
                ALOGE("Debug: Display %llu has only %u h tiles and %u v tiles. Not a true "
                      "tile display or invalid tile h or v locations given.",
                      (unsigned long long)displayId, num_h_tiles, num_v_tiles);
            } else {
                err = mQtiDisplayConfigHidl->SetPowerModeTiled(displayId, hwcMode,
                                                               static_cast<uint32_t>(tile_h_loc),
                                                               static_cast<uint32_t>(tile_v_loc));
                if (NO_ERROR != err) {
                    ALOGE("Debug: DisplayConfig::SetPowerModeTiled() returned error %d", err);
                }
            }
            ALOGI("Debug: Set display = %llu, power mode = %d at tile h loc = %d, tile v "
                  "loc = %d (Has %u h tiles and %u v tiles)",
                  (unsigned long long)displayId, mode, tile_h_loc, tile_v_loc, num_h_tiles,
                  num_v_tiles);
        }
    }
    return err;
}

status_t QtiSurfaceFlingerExtension::qtiBinderSetPanelBrightnessTiled(uint64_t displayId,
                                                                      int32_t level,
                                                                      int32_t tile_h_loc,
                                                                      int32_t tile_v_loc) {
    uint32_t num_h_tiles = 1;
    uint32_t num_v_tiles = 1;
    status_t err = NO_ERROR;

    float levelf = static_cast<float>(level) / 255.0f;
    gui::DisplayBrightness brightness;
    brightness.displayBrightness = levelf;

    if (tile_h_loc < 0) {
        ALOGI("Debug: Set display = %llu, brightness level = %d/255 (%0.2ff)",
              (unsigned long long)displayId, level, levelf);
        if (const auto dispId = DisplayId::fromValue<PhysicalDisplayId>(displayId); dispId) {
            mQtiFlinger->setDisplayBrightness(mQtiFlinger->getPhysicalDisplayToken(dispId.value()),
                                              brightness);
        }
    } else {
        // A regular display has one h tile and one v tile.
        mQtiDisplayConfigHidl->GetDisplayTileCount(displayId, &num_h_tiles, &num_v_tiles);
        if (((num_h_tiles * num_v_tiles) < 2) || static_cast<uint32_t>(tile_h_loc) >= num_h_tiles ||
            static_cast<uint32_t>(tile_v_loc) >= num_v_tiles) {
            ALOGE("Debug: Display %llu has only %u h tiles and %u v tiles. Not a true "
                  "tile display or invalid tile h or v locations given.",
                  (unsigned long long)displayId, num_h_tiles, num_v_tiles);
        } else {
            err = mQtiDisplayConfigHidl->SetPanelBrightnessTiled(displayId,
                                                                 static_cast<uint32_t>(level),
                                                                 static_cast<uint32_t>(tile_h_loc),
                                                                 static_cast<uint32_t>(tile_v_loc));
            if (NO_ERROR != err) {
                ALOGE("Debug: DisplayConfig::SetPanelBrightnessTiled() returned error "
                      "%d",
                      err);
            }
        }
        ALOGI("Debug: Set display = %llu, brightness level = %d/255 (%0.2ff) at tile h "
              "loc = %d, tile v loc = %d (Has %u h tiles and %u v tiles)",
              (unsigned long long)displayId, level, levelf, tile_h_loc, tile_v_loc, num_h_tiles,
              num_v_tiles);
    }
    return err;
}

status_t QtiSurfaceFlingerExtension::qtiBinderSetWideModePreference(uint64_t displayId,
                                                                    int32_t pref) {
    status_t err = NO_ERROR;
    ALOGI("Debug: Set display = %llu, wider-mode preference = %d", (unsigned long long)displayId,
          pref);
    ::DisplayConfig::WiderModePref wider_mode_pref = ::DisplayConfig::WiderModePref::kNoPreference;
    switch (pref) {
        case 1:
            wider_mode_pref = ::DisplayConfig::WiderModePref::kWiderAsyncMode;
            break;
        case 2:
            wider_mode_pref = ::DisplayConfig::WiderModePref::kWiderSyncMode;
            break;
        default:
            // Use default DisplayConfig::WiderModePref::kNoPreference.
            break;
    }
    err = mQtiDisplayConfigHidl->SetWiderModePreference(displayId, wider_mode_pref);
    if (NO_ERROR != err) {
        ALOGE("Debug: DisplayConfig::SetWiderModePreference() returned error %d", err);
    }
    return err;
}

void QtiSurfaceFlingerExtension::qtiSetPowerMode(const sp<IBinder>& displayToken, int mode) {
    sp<DisplayDevice> display = nullptr;
    {
        Mutex::Autolock lock(mQtiFlinger->mStateLock);
        display = (mQtiFlinger->getDisplayDeviceLocked(displayToken));
    }

    if (!display) {
        ALOGE("Attempt to set power mode %d for invalid display token %p", mode,
              displayToken.get());
        return;
    } else if (display->isVirtual()) {
        ALOGW("Attempt to set power mode %d for virtual display", mode);
        return;
    }

    if (mode < 0 || mode > (int)hal::PowerMode::DOZE_SUSPEND) {
        ALOGW("Attempt to set invalid power mode %d", mode);
        return;
    }

    if (!mQtiEnabledIDC) {
        ALOGV("IDisplayConfig AIDL and HIDL are unavaibale, fall back to default setPowerMode");
        mQtiFlinger->setPowerMode(displayToken, mode);
        return;
    }

    hal::PowerMode power_mode = static_cast<hal::PowerMode>(mode);
    const auto displayId = display->getId();
    const auto physicalDisplayId = PhysicalDisplayId::tryCast(displayId);
    if (!physicalDisplayId) {
        ALOGW("Attempt to set invalid displayId");
        return;
    }
    const auto hwcDisplayId =
            mQtiFlinger->getHwComposer().fromPhysicalDisplayId(*physicalDisplayId);
    const auto currentDisplayPowerMode = display->getPowerMode();
    const hal::PowerMode newDisplayPowerMode = static_cast<hal::PowerMode>(mode);
    // Fallback to default power state behavior as HWC does not support power mode override.
    if (!display->qtiGetPowerModeOverrideConfig() ||
        !((currentDisplayPowerMode == hal::PowerMode::OFF &&
           newDisplayPowerMode == hal::PowerMode::ON) ||
          (currentDisplayPowerMode == hal::PowerMode::ON &&
           newDisplayPowerMode == hal::PowerMode::OFF))) {
        ALOGV("HWC does not support power mode override, fall back to default setPowerMode");
        mQtiFlinger->setPowerMode(displayToken, mode);
        return;
    }

    ::DisplayConfig::PowerMode hwcMode = ::DisplayConfig::PowerMode::kOff;
    if (power_mode == hal::PowerMode::ON) {
        hwcMode = ::DisplayConfig::PowerMode::kOn;
    }

    bool step_up = false;
    if (currentDisplayPowerMode == hal::PowerMode::OFF &&
        newDisplayPowerMode == hal::PowerMode::ON) {
        step_up = true;
    }
    // Change hardware state first while stepping up.
    if (step_up) {
        mQtiDisplayConfigHidl->SetPowerMode(static_cast<uint32_t>(*hwcDisplayId), hwcMode);
    }
    // Change SF state now.
    mQtiFlinger->setPowerMode(displayToken, mode);
    // Change hardware state now while stepping down.

    if (!step_up) {
        mQtiDisplayConfigHidl->SetPowerMode(static_cast<uint32_t>(*hwcDisplayId), hwcMode);
    }
}

void QtiSurfaceFlingerExtension::qtiSetPowerModeOverrideConfig(sp<DisplayDevice> display) {
    bool supported = false;
    const auto physicalDisplayId = PhysicalDisplayId::tryCast(display->getId());
    if (physicalDisplayId) {
        const auto hwcDisplayId =
                mQtiFlinger->getHwComposer().fromPhysicalDisplayId(*physicalDisplayId);

        if (mQtiDisplayConfigAidl) {
            mQtiDisplayConfigAidl->isPowerModeOverrideSupported(static_cast<int32_t>(*hwcDisplayId),
                                                                &supported);
            goto end;
        }

        if (mQtiDisplayConfigHidl) {
            mQtiDisplayConfigHidl->IsPowerModeOverrideSupported(static_cast<uint32_t>(
                                                                        *hwcDisplayId),
                                                                &supported);
            goto end;
        }
    }

end:
    if (supported) {
        display->qtiSetPowerModeOverrideConfig(true);
    }
}


void QtiSurfaceFlingerExtension::qtiSetLayerAsMask(uint32_t hwcDisplayId, uint64_t layerId) {
    if (mQtiDisplayConfigAidl) {
        ALOGV("IDisplayConfig AIDL: Set layer %lu as mask for display %d", layerId, hwcDisplayId);
        mQtiDisplayConfigAidl->setLayerAsMask(static_cast<int32_t>(hwcDisplayId),
                                              static_cast<int32_t>(layerId));
    } else if (mQtiDisplayConfigHidl) {
        ALOGV("IDisplayConfig HIDL: Set layer %lu as mask for display %d", layerId, hwcDisplayId);
        mQtiDisplayConfigHidl->SetLayerAsMask(hwcDisplayId, layerId);
    }
}


/*
 * Methods for Virtual, WiFi, and Secure Displays
 */
VirtualDisplayId QtiSurfaceFlingerExtension::qtiAcquireVirtualDisplay(ui::Size resolution,
                                                                      ui::PixelFormat format,
                                                                      bool canAllocateHwcForVDS) {
    auto& generator = mQtiFlinger->mVirtualDisplayIdGenerators.hal;
    if (canAllocateHwcForVDS && generator) {
        if (const auto id = generator->generateId()) {
            if (mQtiFlinger->getHwComposer().allocateVirtualDisplay(*id, resolution, &format)) {
                return *id;
            }

            generator->releaseId(*id);
        } else {
            ALOGW("%s: Exhausted HAL virtual displays", __func__);
        }

        ALOGW("%s: Falling back to GPU virtual display", __func__);
    }

    const auto id = mQtiFlinger->mVirtualDisplayIdGenerators.gpu.generateId();
    LOG_ALWAYS_FATAL_IF(!id, "Failed to generate ID for GPU virtual display");
    return *id;
}

bool QtiSurfaceFlingerExtension::qtiCanAllocateHwcDisplayIdForVDS(const DisplayDeviceState& state) {
    uint64_t usage = 0;
    int status = 0;
    size_t maxVirtualDisplaySize = mQtiFlinger->getHwComposer().getMaxVirtualDisplayDimension();

    ui::Size resolution(0, 0);
    status = state.surface->query(NATIVE_WINDOW_WIDTH, &resolution.width);
    if (status != NO_ERROR) {
        ALOGE("Unable to query width (%d)", status);
        goto cleanup;
    }

    status = state.surface->query(NATIVE_WINDOW_HEIGHT, &resolution.height);
    if (status != NO_ERROR) {
        ALOGE("Unable to query height (%d)", status);
        goto cleanup;
    }

    // Replace with native_window_get_consumer_usage ?
    status = state.surface->getConsumerUsage(&usage);
    if (status != NO_ERROR) {
        ALOGW("Unable to query usage (%d)", status);
        goto cleanup;
    }

    if (maxVirtualDisplaySize == 0 ||
        ((uint64_t)resolution.width <= maxVirtualDisplaySize &&
         (uint64_t)resolution.height <= maxVirtualDisplaySize)) {
        return qtiCanAllocateHwcDisplayIdForVDS(usage);
    }

cleanup:
    return false;
}

bool QtiSurfaceFlingerExtension::qtiCanAllocateHwcDisplayIdForVDS(uint64_t usage) {
    uint64_t flag_mask_pvt_wfd = static_cast<uint64_t>(~0);
    uint64_t flag_mask_hw_video = static_cast<uint64_t>(~0);
    bool mAllowHwcForVDS = mQtiFeatureManager->qtiIsExtensionFeatureEnabled(kHwcForVds);
    bool mAllowHwcForWFD = mQtiFeatureManager->qtiIsExtensionFeatureEnabled(kHwcForWfd);

    // Reserve hardware acceleration for WFD use-case
    // GRALLOC_USAGE_PRIVATE_WFD + GRALLOC_USAGE_HW_VIDEO_ENCODER = WFD using HW composer.
    flag_mask_pvt_wfd = GRALLOC_USAGE_PRIVATE_WFD;
    flag_mask_hw_video = GRALLOC_USAGE_HW_VIDEO_ENCODER;
    // GRALLOC_USAGE_PRIVATE_WFD + GRALLOC_USAGE_SW_READ_OFTEN
    // WFD using GLES (directstreaming).
    mQtiSDirectStreaming =
            ((usage & GRALLOC_USAGE_PRIVATE_WFD) && (usage & GRALLOC_USAGE_SW_READ_OFTEN));
    bool isWfd = (usage & flag_mask_pvt_wfd) && (usage & flag_mask_hw_video);

    // Enabling only the vendor property would allow WFD to use HWC
    // Enabling both the aosp and vendor properties would allow all other VDS to use HWC
    // Disabling both would set all virtual displays to fall back to GPU
    // In vendor frozen targets, allow WFD to use HWC without any property settings.
    bool canAllocate = mAllowHwcForVDS || (isWfd && mAllowHwcForWFD) ||
            (isWfd && mQtiFirstApiLevel < __ANDROID_API_T__);

    if (canAllocate) {
        mQtiFlinger->enableHalVirtualDisplays(true);
    }

    return canAllocate;
}

void QtiSurfaceFlingerExtension::qtiCheckVirtualDisplayHint(const Vector<DisplayState>& displays) {
    bool asyncVdsCreationSupported =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(kAsyncVdsCreationSupported);
    if (!asyncVdsCreationSupported) {
        return;
    }

    for (const DisplayState& s : displays) {
        const ssize_t index = mQtiFlinger->mCurrentState.displays.indexOfKey(s.token);
        if (index < 0) continue;

        DisplayDeviceState& state =
                mQtiFlinger->mCurrentState.displays.editValueAt(static_cast<size_t>(index));
        const uint32_t what = s.what;
        if (what & DisplayState::eSurfaceChanged) {
            if (IInterface::asBinder(state.surface) != IInterface::asBinder(s.surface)) {
                if (state.isVirtual() && s.surface != nullptr) {
                    int width = 0;
                    int status = s.surface->query(NATIVE_WINDOW_WIDTH, &width);
                    ALOGE_IF(status != NO_ERROR, "Unable to query width (%d)", status);
                    int height = 0;
                    status = s.surface->query(NATIVE_WINDOW_HEIGHT, &height);
                    ALOGE_IF(status != NO_ERROR, "Unable to query height (%d)", status);
                    int format = 0;
                    status = s.surface->query(NATIVE_WINDOW_FORMAT, &format);
                    ALOGE_IF(status != NO_ERROR, "Unable to query format (%d)", status);
                    size_t maxVirtualDisplaySize =
                            mQtiFlinger->getHwComposer().getMaxVirtualDisplayDimension();

                    // Create VDS if IDisplayConfig AIDL/HIDL is present and if
                    // the resolution is within the maximum allowed dimension for VDS
                    if (mQtiEnabledIDC && (maxVirtualDisplaySize == 0 ||
                         ((uint64_t)width <= maxVirtualDisplaySize &&
                          (uint64_t)height <= maxVirtualDisplaySize))) {
                        uint64_t usage = 0;
                        // Replace with native_window_get_consumer_usage ?
                        status = s.surface->getConsumerUsage(&usage);
                        ALOGW_IF(status != NO_ERROR, "Unable to query usage (%d)", status);
                        if ((status == NO_ERROR) && qtiCanAllocateHwcDisplayIdForVDS(usage)) {
                            qtiCreateVirtualDisplay(width, height, format);
                            return;
                        }
                    }
                }
            }
        }
    }
}

void QtiSurfaceFlingerExtension::qtiCreateVirtualDisplay(int width, int height, int format) {
    if (!mQtiEnabledIDC) {
        ALOGV("IDisplayConfig AIDL and HIDL are not available.");
        return;
    }

// Use either IDisplayConfig AIDL or HIDL
    if (mQtiDisplayConfigAidl) {
        mQtiDisplayConfigAidl->createVirtualDisplay(width, height, format);
        return;
    }

    if (mQtiDisplayConfigHidl) {
        mQtiDisplayConfigHidl->CreateVirtualDisplay((uint32_t)width, (uint32_t)height, format);
        return;
    }
}

void QtiSurfaceFlingerExtension::qtiHasProtectedLayer(bool* hasProtectedLayer) {
    // Surface flinger captures individual screen shot for each display
    // This will lead consumption of high GPU secure memory in case
    // of secure video use cases and cause out of memory.
    Mutex::Autolock lock(mQtiFlinger->mStateLock);
    if (mQtiFlinger->mDisplays.size() > 1) {
        *hasProtectedLayer = false;
    }
}

bool QtiSurfaceFlingerExtension::qtiIsSecureDisplay(sp<const GraphicBuffer> buffer) {
    return buffer && (buffer->getUsage() & GRALLOC_USAGE_PRIVATE_SECURE_DISPLAY);
}

bool QtiSurfaceFlingerExtension::qtiIsSecureCamera(sp<const GraphicBuffer> buffer) {
    bool protected_buffer = buffer && (buffer->getUsage() & BufferUsage::PROTECTED);
    bool camera_output = buffer && (buffer->getUsage() & BufferUsage::CAMERA_OUTPUT);
    return protected_buffer && camera_output;
}

/*
 * Methods for SmoMo Interface
 */
void QtiSurfaceFlingerExtension::qtiCreateSmomoInstance(const DisplayDeviceState& state) {
    const auto displayOpt = mQtiFlinger->mPhysicalDisplays.get(state.physical->id);
    const auto& displayObject = displayOpt->get();
    const auto& snapshot = displayObject.snapshot();

    if (state.isVirtual() || snapshot.connectionType() == ui::DisplayConnectionType::External) {
        return;
    }

    if (!mQtiFeatureManager->qtiIsExtensionFeatureEnabled(kSmomo)) {
        ALOGI("Smomo is disabled through property");
        return;
    }

    SmomoInfo smomoInfo;
    smomoInfo.displayId = state.physical->hwcDisplayId;
    smomoInfo.layerStackId = state.layerStack.id;
    smomoInfo.active = true;
    smomo::DisplayInfo displayInfo;
    displayInfo.display_id = static_cast<int32_t>(state.physical->hwcDisplayId);
    displayInfo.is_primary = state.physical->hwcDisplayId == 0;
    displayInfo.type = smomo::kBuiltin;
    bool ret = mQtiComposerExtnIntf->CreateSmomoExtn(&smomoInfo.smoMo, displayInfo);
    if (!ret) {
        ALOGI("Unable to create smomo extension for display: %d", displayInfo.display_id);
        return;
    }

    mQtiSmomoInstances.push_back(smomoInfo);
    // Set refresh rates for primary display's instance.
    smomoInfo.smoMo->SetChangeRefreshRateCallback(
            [this](int32_t refreshRate) { qtiSetRefreshRateTo(refreshRate); });

    const auto display = mQtiFlinger->getDefaultDisplayDeviceLocked();
    qtiSetRefreshRates(display->getPhysicalId());

    if (mQtiSmomoInstances.size() > 1) {
        // Disable DRC on all instances.
        for (auto& instance : mQtiSmomoInstances) {
            instance.smoMo->SetRefreshRateChangeStatus(false);
        }
    }

    ALOGI("SmoMo is enabled for display: %d", displayInfo.display_id);
}

void QtiSurfaceFlingerExtension::qtiDestroySmomoInstance(const sp<DisplayDevice>& display) {
    uint32_t hwcDisplayId = 0;
    if (!qtiGetHwcDisplayId(display, &hwcDisplayId)) {
        return;
    }

    mQtiSmomoInstances.erase(std::remove_if(mQtiSmomoInstances.begin(), mQtiSmomoInstances.end(),
                                            [&](SmomoInfo const& smomoInfo) {
                                                return smomoInfo.displayId == hwcDisplayId;
                                            }),
                             mQtiSmomoInstances.end());

    // Enable DRC if only one instance is active.
    if (mQtiSmomoInstances.size() == 1) {
        // Disable DRC on all instances.
        mQtiSmomoInstances.at(0).smoMo->SetRefreshRateChangeStatus(false);
    }

    ALOGI("SmoMo is destroyed for display: %s", to_string(display->getId()).c_str());
}

SmomoIntf* QtiSurfaceFlingerExtension::qtiGetSmomoInstance(const uint32_t layerStackId) const {
    SmomoIntf* smoMo = nullptr;

    if (mQtiSmomoInstances.size() == 1) {
        smoMo = mQtiSmomoInstances.back().smoMo;
        return smoMo;
    }

    for (auto& instance : mQtiSmomoInstances) {
        if (instance.layerStackId == layerStackId) {
            smoMo = instance.smoMo;
            break;
        }
    }

    return smoMo;
}

void QtiSurfaceFlingerExtension::qtiSetRefreshRates(PhysicalDisplayId displayId) {
    std::vector<float> refreshRates;

    const auto modeOpt = [&]() {
        ConditionalLock lock(mQtiFlinger->mStateLock,
                             std::this_thread::get_id() != mQtiFlinger->mMainThreadId);
        return mQtiFlinger->mPhysicalDisplays.get(displayId)
                .transform(&display::PhysicalDisplay::snapshotRef)
                .and_then([&](const display::DisplaySnapshot& snapshot) {
                    const auto& displayModes = snapshot.displayModes();
                    for (const auto& [id, mode] : displayModes) {
                        refreshRates.push_back(mode->getFps().getValue());
                    }
                    return std::optional<bool>(true);
                });
    }();

    if (!modeOpt) {
        ALOGW("qtiSetRefreshRates failed on: %s", to_string(displayId).c_str());
        return;
    }

    SmomoIntf* smoMo = nullptr;
    for (auto& instance : mQtiSmomoInstances) {
        smoMo = instance.smoMo;
        if (smoMo == nullptr) {
            continue;
        }
        smoMo->SetDisplayRefreshRates(refreshRates);
    }
}

void QtiSurfaceFlingerExtension::qtiSetRefreshRateTo(int32_t refreshRate) {
    sp<display::DisplayToken> displayToken;

    const auto modeIdOpt = [&]() {
        ConditionalLock lock(mQtiFlinger->mStateLock,
                             std::this_thread::get_id() != mQtiFlinger->mMainThreadId);

        auto setRefreshRate = Fps::fromValue(refreshRate);
        PhysicalDisplayId displayId = mQtiFlinger->getPrimaryDisplayIdLocked();
        displayToken = mQtiFlinger->getPrimaryDisplayTokenLocked();

        return mQtiFlinger->mPhysicalDisplays.get(displayId)
                .transform(&display::PhysicalDisplay::snapshotRef)
                .and_then([&](const display::DisplaySnapshot& snapshot)
                                  -> std::optional<DisplayModeId> {
                    const auto& displayModes = snapshot.displayModes();
                    for (const auto& [id, mode] : displayModes) {
                        if (isApproxEqual(mode->getFps(), setRefreshRate)) {
                            return id;
                        }
                    }
                    return std::nullopt;
                });
    }();

    if (!modeIdOpt) {
        ALOGW("qtiSetRefreshRateTo: Invalid Mode ID");
        return;
    }

    const status_t result = mQtiFlinger->setActiveModeFromBackdoor(displayToken, *modeIdOpt);
    if (result != NO_ERROR) {
        ALOGW("qtiSetRefreshRateTo: Failed to setActiveMode");
    }

    return;
}

void QtiSurfaceFlingerExtension::qtiSyncToDisplayHardware() NO_THREAD_SAFETY_ANALYSIS {
    ATRACE_CALL();

    const uint32_t layerStackId = mQtiFlinger->getDefaultDisplayDeviceLocked()->getLayerStack().id;
    if (SmomoIntf* smoMo = qtiGetSmomoInstance(layerStackId)) {
        nsecs_t timestamp = 0;
        // Get the previous frame fence since AOSP deprecated the previousFrameFence() API
        auto &scheduler = mQtiFlinger->mScheduler;
        auto prevFrameFence = scheduler->vsyncModulator().getVsyncConfig().sfOffset >= 0
                ? mQtiFlinger->mPreviousPresentFences[0]
                : mQtiFlinger->mPreviousPresentFences[1];
        bool needResync = smoMo->SyncToDisplay(prevFrameFence.fence, &timestamp);
        ALOGV("needResync = %d, timestamp = %" PRId64, needResync, timestamp);
    }
}

void QtiSurfaceFlingerExtension::qtiUpdateSmomoState() {
    ATRACE_NAME("SmoMoUpdateState");
    Mutex::Autolock lock(mQtiFlinger->mStateLock);
    // Check if smomo instances exist.
    if (!mQtiSmomoInstances.size()) {
        return;
    }

    if (mQtiSmomoInstances.size() > 1) {
        mQtiFlinger->mDrawingState.traverse(
                [&](Layer* layer) { layer->qtiSetSmomoLayerStackId(); });
    }

    // Disable smomo if external or virtual is connected.
    bool enableSmomo = mQtiSmomoInstances.size() == mQtiFlinger->mDisplays.size();
    int fps = 0;
    int content_fps = 0;
    int numActiveDisplays = 0;
    for (auto& instance : mQtiSmomoInstances) {
        SmomoIntf* smoMo = instance.smoMo;
        sp<DisplayDevice> device = nullptr;

        for (const auto& [token, displayDevice] : mQtiFlinger->mDisplays) {
            uint32_t hwcDisplayId;
            if (!qtiGetHwcDisplayId(displayDevice, &hwcDisplayId)) {
                continue;
            }
            if (hwcDisplayId == instance.displayId) {
                device = displayDevice;
                break;
            }
        }

        instance.active = device->getPowerMode() != hal::PowerMode::OFF;
        if (!instance.active) {
            continue;
        }

        std::vector<smomo::SmomoLayerStats> layers;
        if (enableSmomo) {
            bool visibleLayersInfo = (mQtiFlinger->mLayersWithQueuedFrames.size() != 0);

            if (visibleLayersInfo) {
                for (const auto& layer : mQtiFlinger->mLayersWithQueuedFrames) {
                    smomo::SmomoLayerStats layerStats;
                    layerStats.name = layer->getDebugName();
                    layerStats.id = layer->getSequence();
                    layers.push_back(layerStats);
                }
            }

            fps = device->getActiveMode().fps.getIntValue();
        }

        smoMo->UpdateSmomoState(layers, fps);

        content_fps = smoMo->GetFrameRate();
        numActiveDisplays++;
    }

    if (numActiveDisplays == 1) {
        bool is_valid_content_fps = false;
        if (mQtiSmomoInstances.size() == 1) {
            if (content_fps > 0) {
                if (mQtiFlinger->mLayersWithQueuedFrames.size() == 1 && !mQtiUiLayerFrameCount) {
                    is_valid_content_fps = true;
                }
                if (mQtiFlinger->mLayersWithQueuedFrames.size() > 1) {
                    mQtiUiLayerFrameCount = fps;
                }
                if (mQtiUiLayerFrameCount > 0) {
                    mQtiUiLayerFrameCount--;
                }
            } else {
                mQtiUiLayerFrameCount = fps;
            }
        }
        qtiSetContentFps(is_valid_content_fps ? static_cast<uint32_t>(content_fps)
                                              : static_cast<uint32_t>(fps));
    }

    // Disable DRC if active displays is more than 1.
    for (auto& instance : mQtiSmomoInstances) {
        instance.smoMo->SetRefreshRateChangeStatus((numActiveDisplays == 1));
    }
}

void QtiSurfaceFlingerExtension::qtiUpdateSmomoLayerInfo(TransactionState& ts,
                                                         int64_t desiredPresentTime,
                                                         bool isAutoTimestamp,
                                                         uint64_t transactionId) {
    ts.traverseStatesWithBuffers([&](const layer_state_t& state) {
        sp<Layer> layer = LayerHandle::getLayer(state.surface);

        SmomoIntf* smoMo = nullptr;
        if (layer != nullptr) {
            smoMo = qtiGetSmomoInstance(layer->qtiGetSmomoLayerStackId());
        }

        if (smoMo) {
            smomo::SmomoBufferStats bufferStats;
            bufferStats.id = layer->getSequence();
            bufferStats.auto_timestamp = isAutoTimestamp;
            bufferStats.timestamp = desiredPresentTime;
            bufferStats.dequeue_latency = 0;
            bufferStats.key = desiredPresentTime;
#ifdef TIMED_RENDERING_METADATA_FEATURE
            auto buffer = mQtiFlinger->getExternalTextureFromBufferData(*state.bufferData,
                                                                        layer->getDebugName(),
                                                                        transactionId);
            if (buffer && buffer->getBuffer()) {
                bufferStats.buffer_hnd = buffer->getBuffer()->handle;
            }
#endif
            smoMo->CollectLayerStats(bufferStats);

            const auto &schedule = mQtiFlinger->mScheduler->getVsyncSchedule();
            auto vsyncTime =
                    schedule.getTracker().nextAnticipatedVSyncTimeFrom(SYSTEM_TIME_MONOTONIC);

            if (smoMo->FrameIsLate(bufferStats.id, vsyncTime)) {
                qtiScheduleCompositeImmed();
            }
        }
    });
}

void QtiSurfaceFlingerExtension::qtiScheduleCompositeImmed() {
    mQtiFlinger->mMustComposite = true;
    mQtiFlinger->mScheduler->resetIdleTimer();
    qtiNotifyDisplayUpdateImminent();
    mQtiFlinger->mScheduler->qtiScheduleFrameImmed();
}

void QtiSurfaceFlingerExtension::qtiSetPresentTime(uint32_t layerStackId, int sequence,
                                                   nsecs_t desiredPresentTime) {
    SmomoIntf* smoMo = qtiGetSmomoInstance(layerStackId);
    if (smoMo) {
        smoMo->SetPresentTime(sequence, desiredPresentTime);
    }
}

void QtiSurfaceFlingerExtension::qtiOnVsync(nsecs_t expectedVsyncTime) {
    SmomoIntf* smoMo = nullptr;
    for (auto& instance : mQtiSmomoInstances) {
        smoMo = instance.smoMo;
        if (smoMo) {
            smoMo->OnVsync(expectedVsyncTime);
        }
    }
}

bool QtiSurfaceFlingerExtension::qtiIsFrameEarly(uint32_t layerStackId, int sequence,
                                                 nsecs_t desiredPresentTime) {
    SmomoIntf* smoMo = qtiGetSmomoInstance(layerStackId);
    bool isEarly = false;
    if (smoMo) {
        isEarly = smoMo->FrameIsEarly(sequence, desiredPresentTime);
    }

    return isEarly;
}

void QtiSurfaceFlingerExtension::qtiUpdateLayerState(int numLayers) {
    bool mSplitLayerExt = mQtiFeatureManager->qtiIsExtensionFeatureEnabled(kSplitLayerExtension);

    if (mSplitLayerExt && mQtiLayerExt) {
        if (mQtiVisibleLayerInfo.layerName.size() != 0) {
            mQtiLayerExt->UpdateLayerState(mQtiVisibleLayerInfo.layerName, numLayers);
        }
    }
    mQtiVisibleLayerInfo.layerName.clear();
    mQtiVisibleLayerInfo.layerSequence.clear();
}

void QtiSurfaceFlingerExtension::qtiUpdateSmomoLayerStackId(hal::HWDisplayId hwcDisplayId,
                                                            uint32_t curLayerStackId,
                                                            uint32_t drawLayerStackId) {
    for (auto& instance : mQtiSmomoInstances) {
        if ((instance.displayId == hwcDisplayId) && instance.layerStackId == drawLayerStackId) {
            instance.layerStackId = curLayerStackId;
            break;
        }
    }
}

uint32_t QtiSurfaceFlingerExtension::qtiGetLayerClass(std::string mName) {
    if (mQtiLayerExt) {
        uint32_t layerClass = static_cast<uint32_t>(mQtiLayerExt->GetLayerClass(mName));
        return layerClass;
    }
    ALOGV("%s: QtiLayerExtension is not enabled, setting layer class to 0");
    return 0;
}

/*
 * Methods for speculative fence
 */
void QtiSurfaceFlingerExtension::qtiStartUnifiedDraw() {
    if (mQtiDisplayExtnIntf) {
        // Displays hotplugged at this point.
        for (const auto& display : mQtiDisplaysList) {
            qtiTryDrawMethod(display);
        }
    }
}

void QtiSurfaceFlingerExtension::qtiTryDrawMethod(sp<DisplayDevice> display) {
    if (!mQtiHWComposerExtnIntf || !mQtiDisplayExtnIntf) {
        return;
    }

    const auto id = HalDisplayId::tryCast(display->getId());
    if (id) {
        uint32_t hwcDisplayId;
        if (!qtiGetHwcDisplayId(display, &hwcDisplayId)) {
            return;
        }
        ALOGI("calling TryUnifiedDraw for display=%u", hwcDisplayId);
        if (!mQtiDisplayExtnIntf
                     ->TryUnifiedDraw(hwcDisplayId,
                                      static_cast<int32_t>(
                                              mQtiFlinger->maxFrameBufferAcquiredBuffers))) {
            ALOGI("Calling tryDrawMethod for display=%u", hwcDisplayId);
            mQtiHWComposerExtnIntf
                    ->qtiTryDrawMethod(*id,
                                       static_cast<uint32_t>(
                                               IQtiComposerClient::DrawMethod::UNIFIED_DRAW));
        }
    }
}

void QtiSurfaceFlingerExtension::qtiEndUnifiedDraw(uint32_t hwcDisplayId) {
    if (mQtiDisplayExtnIntf) {
        mQtiDisplayExtnIntf->EndUnifiedDraw(hwcDisplayId);
    }
}

/*
 * Methods internal to QtiSurfaceFlingerExtension
 */
bool QtiSurfaceFlingerExtension::qtiIsInternalDisplay(const sp<DisplayDevice>& display) {
    if (display) {
        const auto displayOpt = mQtiFlinger->mPhysicalDisplays.get(display->getPhysicalId());
        const auto& physicalDisplay = displayOpt->get();
        const auto& snapshot = physicalDisplay.snapshot();

        const auto connectionType = snapshot.connectionType();
        return (connectionType == ui::DisplayConnectionType::Internal);
    }
    return false;
}

void QtiSurfaceFlingerExtension::qtiSetupDisplayExtnFeatures() {
    bool enableEarlyWakeUp =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kEarlyWakeUp);
    bool enableDynamicSfIdle =
            mQtiFeatureManager->qtiIsExtensionFeatureEnabled(QtiFeature::kDynamicSfIdle);
    if (enableEarlyWakeUp || enableDynamicSfIdle) {
        for (const auto& display : mQtiDisplaysList) {
            // Register Internal Physical Displays
            if (qtiIsInternalDisplay(display)) {
                uint32_t hwcDisplayId;
                if (qtiGetHwcDisplayId(display, &hwcDisplayId)) {
                    const auto displayId =
                            DisplayId::fromValue<PhysicalDisplayId>(display->getId().value);
                    if (displayId) {
                        auto configId =
                                mQtiFlinger->getHwComposer().getActiveMode(displayId.value());
                        LOG_ALWAYS_FATAL_IF(!configId, "HWC returned no active config");
                        qtiUpdateDisplayExtension(hwcDisplayId, *configId, true);
                        if (enableDynamicSfIdle && display->isPrimary()) {
                            // TODO(rmedel): setupIdleTimeoutHandling(hwcDisplayId);
                        }
                    }
                }
            }
        }
    }
}

/*
 * LayerExtWrapper class
 */
bool LayerExtWrapper::init() {
    mLayerExtLibHandle = dlopen(LAYER_EXTN_LIBRARY_NAME, RTLD_NOW);
    if (!mLayerExtLibHandle) {
        ALOGE("Unable to open layer ext lib: %s", dlerror());
        return false;
    }

    mLayerExtCreateFunc = reinterpret_cast<CreateLayerExtnFuncPtr>(
            dlsym(mLayerExtLibHandle, CREATE_LAYER_EXTN_INTERFACE));
    mLayerExtDestroyFunc = reinterpret_cast<DestroyLayerExtnFuncPtr>(
            dlsym(mLayerExtLibHandle, DESTROY_LAYER_EXTN_INTERFACE));

    if (!mLayerExtCreateFunc || !mLayerExtDestroyFunc) {
        ALOGE("Can't load layer ext symbols: %s", dlerror());
        dlclose(mLayerExtLibHandle);
        return false;
    }

    if (!mLayerExtCreateFunc(LAYER_EXTN_VERSION_TAG, &mInst)) {
        ALOGE("Unable to create layer ext interface");
        dlclose(mLayerExtLibHandle);
        return false;
    }

    return true;
}

LayerExtWrapper::~LayerExtWrapper() {
    if (mInst) {
        mLayerExtDestroyFunc(mInst);
    }

    if (mLayerExtLibHandle) {
        dlclose(mLayerExtLibHandle);
    }
}

} // namespace android::surfaceflingerextension
