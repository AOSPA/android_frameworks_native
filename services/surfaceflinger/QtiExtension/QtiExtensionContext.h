/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <compositionengine/CompositionEngine.h>
#include <cstddef>
#include "QtiHWComposerExtensionIntf.h"
#include "QtiSurfaceFlingerExtensionIntf.h"

using android::compositionengine::CompositionEngine;

namespace android::surfaceflingerextension {
// Singleton for surfaceflinger extension
class QtiExtensionContext {
public:
    QtiExtensionContext(const QtiExtensionContext&) = delete;
    QtiExtensionContext& operator=(const QtiExtensionContext&) = delete;
    static QtiExtensionContext& instance() {
        static QtiExtensionContext context;
        return context;
    }

    void setCompositionEngine(CompositionEngine* val) { mQtiCompEngine = val; }
    CompositionEngine* getCompositionEngine() { return mQtiCompEngine; }

    void setQtiSurfaceFlingerExtn(QtiSurfaceFlingerExtensionIntf* val) {
        mQtiSurfaceFlingerExtn = val;
    }
    QtiSurfaceFlingerExtensionIntf* getQtiSurfaceFlingerExtn() {
        return mQtiSurfaceFlingerExtn;
    }

    QtiHWComposerExtensionIntf* getQtiHWComposerExtension() {
        if (mQtiSurfaceFlingerExtn) {
            return mQtiSurfaceFlingerExtn->qtiGetHWComposerExtensionIntf();
        }
        return nullptr;
    }
    composer::DisplayExtnIntf* getDisplayExtension() {
        if (mQtiSurfaceFlingerExtn) {
            return mQtiSurfaceFlingerExtn->qtiGetDisplayExtn();
        }
        return nullptr;
     }


private:
    QtiExtensionContext() = default;
    ~QtiExtensionContext() = default;

    CompositionEngine* mQtiCompEngine = nullptr;
    QtiSurfaceFlingerExtensionIntf* mQtiSurfaceFlingerExtn = nullptr;
};

} // namespace android::surfaceflingerextension
