/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <aidl/vendor/qti/hardware/display/config/IDisplayConfig.h>
#include <aidl/vendor/qti/hardware/display/config/IDisplayConfigCallback.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <config/client_interface.h>
#include <log/log.h>
#include "QtiSurfaceFlingerExtensionIntf.h"

using aidl::vendor::qti::hardware::display::config::IDisplayConfig;
using std::string;

namespace DisplayConfig {
class ClientInterface;
} // namespace DisplayConfig

namespace android {

namespace surfaceflingerextension {

class QtiSurfaceFlingerExtension;

class QtiFeatureManager {
public:
    QtiFeatureManager(QtiSurfaceFlingerExtension* extension);
    ~QtiFeatureManager() = default;

    void qtiInit();
    void qtiPostInit();
    void qtiSetIDisplayConfig(std::shared_ptr<IDisplayConfig> aidl);
    void qtiSetIDisplayConfig(::DisplayConfig::ClientInterface* hidl);
    bool qtiIsExtensionFeatureEnabled(QtiFeature feature);

private:
    string qtiGetPropName(QtiFeature feature);

    QtiSurfaceFlingerExtension* mQtiSFExtension = nullptr;

    /*
     * Properties used to identify if a feature is enabled/disabled.
     */
    bool mQtiAllowHwcForVDS = false;
    bool mQtiAllowHwcForWFD = false;
    bool mQtiAsyncVdsCreationSupported = false;
    bool mQtiEnableDynamicSfIdle = false;
    bool mQtiEnableEarlyWakeUp = false;
    bool mQtiEnableSmomo = false;
    bool mQtiEnableSpecFence = false;
    bool mQtiLatchMediaContent = false;
    bool mQtiPluggableVsyncPrioritized = false;
    bool mQtiUseAdvanceSfOffset = false;
    bool mQtiUseFbScaling = false;
    bool mQtiUseLayerExt = false;
    bool mQtiUseQsyncIdle = false;
    bool mQtiUseSplitLayerExt = false;
    bool mQtiUseWorkDurations = false;
    bool mQtiVsyncSourceReliableOnDoze = false;
    bool mQtiEnableSmomoOptimalRefreshRate = false;
    bool mQtiAllowIdleFallback = false;
};

} // namespace surfaceflingerextension
} // namespace android
