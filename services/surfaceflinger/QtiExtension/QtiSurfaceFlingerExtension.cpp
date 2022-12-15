/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0

#include "QtiSurfaceFlingerExtension.h"

#include <aidl/vendor/qti/hardware/display/config/IDisplayConfig.h>
#include <aidl/vendor/qti/hardware/display/config/IDisplayConfigCallback.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <composer_extn_intf.h>
#include <config/client_interface.h>

#include <Scheduler/VsyncConfiguration.h>
#include "QtiSurfaceFlingerExtension.h"

using aidl::vendor::qti::hardware::display::config::IDisplayConfig;
using PerfHintType = composer::PerfHintType;
using VsyncConfiguration = android::scheduler::VsyncConfiguration;
using VsyncModulator = android::scheduler::VsyncModulator;

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

#ifdef QTI_DISPLAY_AIDL_CONFIG
std::shared_ptr<IDisplayConfig> mQtiDisplayConfigAidl = nullptr;
#endif

#ifdef QTI_DISPLAY_HIDL_CONFIG
::DisplayConfig::ClientInterface* mQtiDisplayConfigHidl = nullptr;
#endif

QtiSurfaceFlingerExtensionIntf* qtiCreateSurfaceFlingerExtension() {
#ifdef QTI_DISPLAY_EXTENSION
    bool mQtiEnableDisplayExtn =
            base::GetBoolProperty("vendor.display.enable_display_extensions", false);
    if (mQtiEnableDisplayExtn) {
        ALOGI("Enabling QTI extensions ...");
        return new QtiSurfaceFlingerExtension();
    }
#endif

    ALOGI("Enabling GSI in QSSI ...");
    return new QtiNullExtension();
}

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
        mQtiFeatureManager->init();
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
}

QtiSurfaceFlingerExtensionIntf* QtiSurfaceFlingerExtension::qtiPostInit(
        android::impl::HWComposer& hwc, Hwc2::impl::PowerAdvisor* powerAdvisor,
        scheduler::VsyncConfiguration* vsyncConfig) {
#ifdef QTI_DISPLAY_AIDL_CONFIG
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
            mQtiEnabledIDC = true;
        }
    }
#endif

#ifdef QTI_DISPLAY_HIDL_CONFIG
    int ret = ::DisplayConfig::ClientInterface::Create("SurfaceFlinger" + std::to_string(0),
                                                       nullptr, &mQtiDisplayConfigHidl);
    if (ret || !mQtiDisplayConfigHidl) {
        ALOGE("DisplayConfig HIDL not present");
        mQtiDisplayConfigHidl = nullptr;
    } else {
        ALOGI("Initialized DisplayConfig HIDL %p successfully", mQtiDisplayConfigHidl);
        mQtiEnabledIDC = true;
    }
#endif

    if (mQtiFeatureManager) {
#ifdef QTI_DISPLAY_AIDL_CONFIG
        mQtiFeatureManager->qtiSetIDisplayConfig(mQtiDisplayConfigAidl);
#endif

#ifdef QTI_DISPLAY_HIDL_CONFIG
        mQtiFeatureManager->qtiSetIDisplayConfig(mQtiDisplayConfigHidl);
#endif
    }

    // When both IDisplayConfig AIDL and HIDL are not available, behave similar to GSI mode
    if (!mQtiEnabledIDC) {
        ALOGW("DisplayConfig HIDL and AIDL are both unavailable - disabling composer extensions");
        return new QtiNullExtension();
    }

    mQtiHWComposerExtn = new QtiHWComposerExtension(hwc);
    mQtiPowerAdvisorExtn = new QtiPowerAdvisorExtension(powerAdvisor);

    qtiSetVsyncConfiguration(vsyncConfig);
    qtiSetupDisplayExtnFeatures();
    qtiUpdateVsyncConfiguration();
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
        const auto halDisplayId = mQtiHWComposerExtn->qtiFromVirtualDisplayId(*virtualDisplayId);
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

    if (qtiIsInternalDisplay) {
        bool qtiIsConnected = (connection == hal::Connection::CONNECTED);
        auto qtiActiveConfigId = mQtiFlinger->getHwComposer().getActiveMode(id);
        LOG_ALWAYS_FATAL_IF(!qtiActiveConfigId, "HWC returned no active config");
        qtiUpdateDisplayExtension(hwcDisplayId, *qtiActiveConfigId, qtiIsConnected);
    }
}

void QtiSurfaceFlingerExtension::qtiUpdateOnComposerHalHotplug(hal::HWDisplayId hwcDisplayId,
                                                               hal::Connection connection) {
    // QTI: Update QTI Extension's displays list when a display is disconnected
    if (connection != hal::Connection::CONNECTED) {
        const std::optional<DisplayIdentificationInfo> info =
                mQtiFlinger->getHwComposer().onHotplug(hwcDisplayId, connection);
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

        /* TODO(rmedel): Enable setDisplayElapseTime
                return mQtiFlinger->getHwComposer()
                        .qtiSetDisplayElapseTime(*id,
                                                 static_cast<uint64_t>(
                                                         timeStamp.time_since_epoch().count()));
        */
    }
    return OK;
}

/*
 *  Methods that call the DisplayExtension APIs.
 */
void QtiSurfaceFlingerExtension::qtiSendCompositorTid() {
#ifdef PASS_COMPOSITOR_TID
    if (!mQtiTidSentSuccessfully && mQtiFlinger->mBootFinished && mQtiDisplayExtnIntf) {
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

        if (useWorkDurations) {
#ifdef DYNAMIC_APP_DURATIONS
            // Update the Work Durations for the given refresh rates in mOffsets map
            g_comp_ext_intf_.phaseOffsetExtnIntf->GetWorkDurationConfigs(
                    &mQtiWorkDurationConfigsMap);
            mQtiWorkDurationsExtn->qtiUpdateWorkDurations(&mQtiWorkDurationConfigsMap);
#endif
        } else {
            // Update the Advanced SF Offsets for the given refresh rates in mOffsets map
            g_comp_ext_intf_.phaseOffsetExtnIntf->GetAdvancedSfOffsets(&mQtiAdvancedSfOffsets);
            mQtiPhaseOffsetsExtn->qtiUpdateSfOffsets(&mQtiAdvancedSfOffsets);
        }

        const auto vsyncConfig = mQtiFlinger->mVsyncModulator->setVsyncConfigSet(
                mQtiFlinger->mVsyncConfiguration->getCurrentConfigs());
        ALOGV("VsyncConfig sfOffset %" PRId64 "\n", vsyncConfig.sfOffset);
        ALOGV("VsyncConfig appOffset %" PRId64 "\n", vsyncConfig.appOffset);
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

    const sp<Fence>& fence = mQtiFlinger->mVsyncModulator->getVsyncConfig().sfOffset > 0
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
    mQtiFlinger->mScheduler->resyncToHardwareVsync(true, Fps::fromPeriodNsecs(period)/*,
                                      true force resync */);
    if (timeStamp > 0) {
        bool periodFlushed = false;
        mQtiFlinger->mScheduler->addResyncSample(timeStamp, period, &periodFlushed);
        if (periodFlushed) {
            mQtiFlinger->modulateVsync(&VsyncModulator::onRefreshRateChangeCompleted);
        }
    }
}

/*
 * Methods that call the IDisplayConfig APIs.
 */
status_t QtiSurfaceFlingerExtension::qtiGetDebugProperty(string prop, string* value) {
#ifdef AIDL_DISPLAY_CONFIG_ENABLED
    auto ret = mQtiDisplayConfigAidl->getDebugProperty(prop, value);
    if (ret.isOk()) {
        ALOGV("GetDebugProperty \"%s\" value %s", prop.c_str(), *value->c_str());
    } else {
        ALOGW("Failed to get property %s", prop.c_str());
    }
#endif
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

#ifdef QTI_DISPLAY_AIDL_CONFIG
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
#elif QTI_DISPLAY_HIDL_CONFIG
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
#endif

    return NO_ERROR;
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

} // namespace android::surfaceflingerextension
