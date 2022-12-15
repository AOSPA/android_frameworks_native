/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SURFACEFLINGER_EXTN_INTF_H__
#define __SURFACEFLINGER_EXTN_INTF_H__

#include <string>

#include "../SurfaceFlinger.h"

using std::string;

namespace android {

namespace surfaceflingerextension {

enum QtiFeature {
    kAdvanceSfOffset = 0,
    kDynamicSfIdle,
    kEarlyWakeUp,
    kFbScaling,
    kHwcForVds,
    kHwcForWfd,
    kLatchMediaContent,
    kLayerExtension,
    kPluggableVsyncPrioritized,
    kQsyncIdle,
    kVsyncSourceReliableOnDoze,
    kWorkDurations,
};

class QtiSurfaceFlingerExtensionIntf {
public:
    virtual ~QtiSurfaceFlingerExtensionIntf() {}

    virtual void qtiInit(SurfaceFlinger* flinger) = 0;
    virtual QtiSurfaceFlingerExtensionIntf* qtiPostInit(
            android::impl::HWComposer& hwc, Hwc2::impl::PowerAdvisor* powerAdvisor,
            scheduler::VsyncConfiguration* vsyncConfig) = 0;
    virtual void qtiSetVsyncConfiguration(scheduler::VsyncConfiguration* vsyncConfig) = 0;
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
                                               hal::Connection connection) = 0;
    virtual void qtiUpdateInternalDisplaysPresentationMode() = 0;

    /*
     * Methods that call the FeatureManager APIs.
     */
    virtual bool qtiIsExtensionFeatureEnabled(QtiFeature feature) = 0;

    /*
     * Methods used by SurfaceFlinger DisplayHardware.
     */
    virtual status_t qtiSetDisplayElapseTime(
            std::chrono::steady_clock::time_point earliestPresentTime) const = 0;

    /*
     * Methods that call the DisplayExtension APIs.
     */
    virtual void qtiSendCompositorTid() = 0;
    virtual void qtiSendInitialFps(uint32_t fps) = 0;
    virtual void qtiNotifyDisplayUpdateImminent() = 0;
    virtual void qtiSetContentFps(uint32_t contentFps) = 0;
    virtual void qtiSetEarlyWakeUpConfig(const sp<DisplayDevice>& display, hal::PowerMode mode) = 0;
    virtual void qtiUpdateVsyncConfiguration() = 0;

    /*
     * Methods that call FrameScheduler APIs.
     */
    virtual void qtiUpdateFrameScheduler() = 0;

    /*
     * Methods that call the IDisplayConfig APIs.
     */
    virtual status_t qtiGetDebugProperty(string prop, string* value) = 0;
    virtual status_t qtiIsSupportedConfigSwitch(const sp<IBinder>& displayToken, int config) = 0;
};

QtiSurfaceFlingerExtensionIntf* qtiCreateSurfaceFlingerExtension();

} // namespace surfaceflingerextension
} // namespace android

#endif
