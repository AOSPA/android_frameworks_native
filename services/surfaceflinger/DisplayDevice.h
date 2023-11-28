/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <android-base/thread_annotations.h>
#include <android/native_window.h>
#include <binder/IBinder.h>
#include <ftl/concat.h>
#include <gui/LayerState.h>
#include <math/mat4.h>
#include <renderengine/RenderEngine.h>
#include <system/window.h>
#include <ui/DisplayId.h>
#include <ui/DisplayIdentification.h>
#include <ui/DisplayState.h>
#include <ui/GraphicTypes.h>
#include <ui/HdrCapabilities.h>
#include <ui/Region.h>
#include <ui/StaticDisplayInfo.h>
#include <ui/Transform.h>
#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>

#include "Display/DisplayModeRequest.h"
#include "DisplayHardware/DisplayMode.h"
#include "DisplayHardware/Hal.h"
#include "DisplayHardware/PowerAdvisor.h"
#include "FrontEnd/DisplayInfo.h"
#include "Scheduler/RefreshRateSelector.h"
#include "ThreadContext.h"
#include "TracedOrdinal.h"
#include "Utils/Dumper.h"
namespace android {

class Fence;
class HWComposer;
class IGraphicBufferProducer;
class Layer;
class RefreshRateOverlay;
class SurfaceFlinger;

struct CompositionInfo;
struct DisplayDeviceCreationArgs;

namespace compositionengine {
class Display;
class DisplaySurface;
} // namespace compositionengine

namespace display {
class DisplaySnapshot;
} // namespace display

/* QTI_BEGIN */
namespace surfaceflingerextension {
class QtiDisplaySurfaceExtensionIntf;
} // namespace surfaceflingerextension

namespace compositionengineextension {
class QtiDisplayExtension;
} // namespace compositionengineextension
/* QTI_END */

class DisplayDevice : public RefBase {
public:
    constexpr static float sDefaultMinLumiance = 0.0;
    constexpr static float sDefaultMaxLumiance = 500.0;
    enum { eReceivesInput = 0x01 };

    explicit DisplayDevice(DisplayDeviceCreationArgs& args);

    // Must be destroyed on the main thread because it may call into HWComposer.
    virtual ~DisplayDevice();

    std::shared_ptr<compositionengine::Display> getCompositionDisplay() const {
        return mCompositionDisplay;
    }

    bool isVirtual() const { return VirtualDisplayId::tryCast(getId()).has_value(); }
    bool isPrimary() const { return mIsPrimary; }

    // isSecure indicates whether this display can be trusted to display
    // secure surfaces.
    bool isSecure() const;

    int getWidth() const;
    int getHeight() const;
    ui::Size getSize() const { return {getWidth(), getHeight()}; }

    void setLayerFilter(ui::LayerFilter);
    void setDisplaySize(int width, int height);
    void setProjection(ui::Rotation orientation, Rect viewport, Rect frame);
    void stageBrightness(float brightness) REQUIRES(kMainThreadContext);
    void persistBrightness(bool needsComposite) REQUIRES(kMainThreadContext);
    bool isBrightnessStale() const REQUIRES(kMainThreadContext);
    void setFlags(uint32_t flags);

    ui::Rotation getPhysicalOrientation() const { return mPhysicalOrientation; }
    ui::Rotation getOrientation() const { return mOrientation; }

    std::optional<float> getStagedBrightness() const REQUIRES(kMainThreadContext);
    ui::Transform::RotationFlags getTransformHint() const;
    const ui::Transform& getTransform() const;
    const Rect& getLayerStackSpaceRect() const;
    const Rect& getOrientedDisplaySpaceRect() const;
    ui::LayerStack getLayerStack() const;
    bool receivesInput() const { return mFlags & eReceivesInput; }

    DisplayId getId() const;

    // Shorthand to upcast the ID of a display whose type is known as a precondition.
    PhysicalDisplayId getPhysicalId() const {
        const auto id = PhysicalDisplayId::tryCast(getId());
        LOG_FATAL_IF(!id);
        return *id;
    }

    VirtualDisplayId getVirtualId() const {
        const auto id = VirtualDisplayId::tryCast(getId());
        LOG_FATAL_IF(!id);
        return *id;
    }

    const wp<IBinder>& getDisplayToken() const { return mDisplayToken; }
    int32_t getSequenceId() const { return mSequenceId; }

    const Region& getUndefinedRegion() const;

    int32_t getSupportedPerFrameMetadata() const;

    bool hasWideColorGamut() const;
    // Whether h/w composer has native support for specific HDR type.
    bool hasHDR10PlusSupport() const;
    bool hasHDR10Support() const;
    bool hasHLGSupport() const;
    bool hasDolbyVisionSupport() const;

    void overrideHdrTypes(const std::vector<ui::Hdr>& hdrTypes);

    // The returned HdrCapabilities is the combination of HDR capabilities from
    // hardware composer and RenderEngine. When the DisplayDevice supports wide
    // color gamut, RenderEngine is able to simulate HDR support in Display P3
    // color space for both PQ and HLG HDR contents. The minimum and maximum
    // luminance will be set to sDefaultMinLumiance and sDefaultMaxLumiance
    // respectively if hardware composer doesn't return meaningful values.
    HdrCapabilities getHdrCapabilities() const;

    // Return true if intent is supported by the display.
    bool hasRenderIntent(ui::RenderIntent intent) const;

    const Rect getBounds() const;
    const Rect bounds() const { return getBounds(); }

    void setDisplayName(const std::string& displayName);
    const std::string& getDisplayName() const { return mDisplayName; }

    surfaceflinger::frontend::DisplayInfo getFrontEndInfo() const;

    /* ------------------------------------------------------------------------
     * Display power mode management.
     */
    std::optional<hardware::graphics::composer::hal::PowerMode> getPowerMode() const;
    void setPowerMode(hardware::graphics::composer::hal::PowerMode mode);
    bool isPoweredOn() const;
    void tracePowerMode();

    // Enables layer caching on this DisplayDevice
    void enableLayerCaching(bool enable);

    ui::Dataspace getCompositionDataSpace() const;

    /* ------------------------------------------------------------------------
     * Display mode management.
     */

    // TODO(b/241285876): Replace ActiveModeInfo and DisplayModeEvent with DisplayModeRequest.
    struct ActiveModeInfo {
        using Event = scheduler::DisplayModeEvent;

        ActiveModeInfo() = default;
        ActiveModeInfo(scheduler::FrameRateMode mode, Event event)
              : modeOpt(std::move(mode)), event(event) {}

        explicit ActiveModeInfo(display::DisplayModeRequest&& request)
              : ActiveModeInfo(std::move(request.mode),
                               request.emitEvent ? Event::Changed : Event::None) {}

        ftl::Optional<scheduler::FrameRateMode> modeOpt;
        Event event = Event::None;

        bool operator!=(const ActiveModeInfo& other) const {
            return modeOpt != other.modeOpt || event != other.event;
        }
    };

    enum class DesiredActiveModeAction {
        None,
        InitiateDisplayModeSwitch,
        InitiateRenderRateSwitch
    };
    DesiredActiveModeAction setDesiredActiveMode(const ActiveModeInfo&, bool force = false)
            EXCLUDES(mActiveModeLock);
    std::optional<ActiveModeInfo> getDesiredActiveMode() const EXCLUDES(mActiveModeLock);
    void clearDesiredActiveModeState() EXCLUDES(mActiveModeLock);
    ActiveModeInfo getUpcomingActiveMode() const REQUIRES(kMainThreadContext) {
        return mUpcomingActiveMode;
    }

    scheduler::FrameRateMode getActiveMode() const REQUIRES(kMainThreadContext) {
        return mRefreshRateSelector->getActiveMode();
    }

    void setActiveMode(DisplayModeId, Fps displayFps, Fps renderFps);

    status_t initiateModeChange(const ActiveModeInfo&,
                                const hal::VsyncPeriodChangeConstraints& constraints,
                                hal::VsyncPeriodChangeTimeline* outTimeline)
            REQUIRES(kMainThreadContext);

    scheduler::RefreshRateSelector& refreshRateSelector() const { return *mRefreshRateSelector; }

    // Extends the lifetime of the RefreshRateSelector, so it can outlive this DisplayDevice.
    std::shared_ptr<scheduler::RefreshRateSelector> holdRefreshRateSelector() const {
        return mRefreshRateSelector;
    }

    // Enables an overlay to be displayed with the current refresh rate
    void enableRefreshRateOverlay(bool enable, bool setByHwc, bool showSpinner, bool showRenderRate,
                                  bool showInMiddle) REQUIRES(kMainThreadContext);
    void updateRefreshRateOverlayRate(Fps displayFps, Fps renderFps, bool setByHwc = false);
    bool isRefreshRateOverlayEnabled() const { return mRefreshRateOverlay != nullptr; }
    bool onKernelTimerChanged(std::optional<DisplayModeId>, bool timerExpired);
    void animateRefreshRateOverlay();

    nsecs_t getVsyncPeriodFromHWC() const;

    Fps getAdjustedRefreshRate() const { return mAdjustedRefreshRate; }

    // Round the requested refresh rate to match a divisor of the pacesetter
    // display's refresh rate. Only supported for virtual displays.
    void adjustRefreshRate(Fps pacesetterDisplayRefreshRate);

    // release HWC resources (if any) for removable displays
    void disconnect();

    void dump(utils::Dumper&) const;

    /* QTI_BEGIN */
    void qtiResetVsyncPeriod();
    void qtiSetPowerModeOverrideConfig(bool supported);
    bool qtiGetPowerModeOverrideConfig() const;
    /* QTI_END */

private:
    const sp<SurfaceFlinger> mFlinger;
    HWComposer& mHwComposer;
    const wp<IBinder> mDisplayToken;
    const int32_t mSequenceId;

    const std::shared_ptr<compositionengine::Display> mCompositionDisplay;

    std::string mDisplayName;
    std::string mActiveModeFPSTrace;
    std::string mActiveModeFPSHwcTrace;
    std::string mRenderFrameRateFPSTrace;

    const ui::Rotation mPhysicalOrientation;
    ui::Rotation mOrientation = ui::ROTATION_0;

    // Allow nullopt as initial power mode.
    using TracedPowerMode = TracedOrdinal<hardware::graphics::composer::hal::PowerMode>;
    std::optional<TracedPowerMode> mPowerMode;

    std::optional<float> mStagedBrightness;
    std::optional<float> mBrightness;

    // TODO(b/182939859): Remove special cases for primary display.
    const bool mIsPrimary;

    uint32_t mFlags = 0;

    // Requested refresh rate in fps, supported only for virtual displays.
    // when this value is non zero, SurfaceFlinger will try to drop frames
    // for virtual displays to match this requested refresh rate.
    const Fps mRequestedRefreshRate;

    // Adjusted refresh rate, rounded to match a divisor of the pacesetter
    // display's refresh rate. Only supported for virtual displays.
    Fps mAdjustedRefreshRate = 0_Hz;

    std::vector<ui::Hdr> mOverrideHdrTypes;

    std::shared_ptr<scheduler::RefreshRateSelector> mRefreshRateSelector;
    std::unique_ptr<RefreshRateOverlay> mRefreshRateOverlay;

    mutable std::mutex mActiveModeLock;
    ActiveModeInfo mDesiredActiveMode GUARDED_BY(mActiveModeLock);
    TracedOrdinal<bool> mDesiredActiveModeChanged GUARDED_BY(mActiveModeLock) =
            {ftl::Concat("DesiredActiveModeChanged-", getId().value).c_str(), false};
    ActiveModeInfo mUpcomingActiveMode GUARDED_BY(kMainThreadContext);

    /* QTI_BEGIN */
    mutable std::mutex mQtiModeLock;
    mutable bool mQtiVsyncPeriodUpdated = true;
    mutable nsecs_t mQtiVsyncPeriod = 0;
    bool mQtiIsPowerModeOverride = false;
    /* QTI_END */
};

struct DisplayDeviceState {
    struct Physical {
        PhysicalDisplayId id;
        hardware::graphics::composer::hal::HWDisplayId hwcDisplayId;
        DisplayModePtr activeMode;

        bool operator==(const Physical& other) const {
            return id == other.id && hwcDisplayId == other.hwcDisplayId;
        }
    };

    bool isVirtual() const { return !physical; }

    int32_t sequenceId = sNextSequenceId++;
    std::optional<Physical> physical;
    sp<IGraphicBufferProducer> surface;
    ui::LayerStack layerStack;
    uint32_t flags = 0;
    Rect layerStackSpaceRect;
    Rect orientedDisplaySpaceRect;
    ui::Rotation orientation = ui::ROTATION_0;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string displayName;
    bool isSecure = false;
    // Refer to DisplayDevice::mRequestedRefreshRate, for virtual display only
    Fps requestedRefreshRate;

private:
    static std::atomic<int32_t> sNextSequenceId;
};

struct DisplayDeviceCreationArgs {
    // We use a constructor to ensure some of the values are set, without
    // assuming a default value.
    DisplayDeviceCreationArgs(const sp<SurfaceFlinger>&, HWComposer& hwComposer,
                              const wp<IBinder>& displayToken,
                              std::shared_ptr<compositionengine::Display>);
    const sp<SurfaceFlinger> flinger;
    HWComposer& hwComposer;
    const wp<IBinder> displayToken;
    const std::shared_ptr<compositionengine::Display> compositionDisplay;
    std::shared_ptr<scheduler::RefreshRateSelector> refreshRateSelector;

    int32_t sequenceId{0};
    bool isSecure{false};
    sp<ANativeWindow> nativeWindow;
    sp<compositionengine::DisplaySurface> displaySurface;
    ui::Rotation physicalOrientation{ui::ROTATION_0};
    bool hasWideColorGamut{false};
    HdrCapabilities hdrCapabilities;
    int32_t supportedPerFrameMetadata{0};
    std::unordered_map<ui::ColorMode, std::vector<ui::RenderIntent>> hwcColorModes;
    std::optional<hardware::graphics::composer::hal::PowerMode> initialPowerMode;
    bool isPrimary{false};
    DisplayModeId activeModeId;
    // QTI_BEGIN
    android::surfaceflingerextension::QtiDisplaySurfaceExtensionIntf* mQtiDSExtnIntf = nullptr;
    // QTI_END
    // Refer to DisplayDevice::mRequestedRefreshRate, for virtual display only
    Fps requestedRefreshRate;
};

} // namespace android
