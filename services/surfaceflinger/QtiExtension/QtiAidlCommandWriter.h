/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <android/hardware/graphics/composer3/ComposerClientWriter.h>
using ::aidl::android::hardware::graphics::composer3::ComposerClientWriter;

#ifndef QTI_COMPOSER3_EXTENSIONS
#define QtiAidlCommandWriter ComposerClientWriter
#else
#include <aidl/vendor/qti/hardware/display/composer3/IQtiComposer3Client.h>

using aidl::vendor::qti::hardware::display::composer3::IQtiComposer3Client;
using aidl::vendor::qti::hardware::display::composer3::QtiDisplayCommand;
using aidl::vendor::qti::hardware::display::composer3::QtiDrawMethod;
using aidl::vendor::qti::hardware::display::composer3::QtiLayerCommand;
using aidl::vendor::qti::hardware::display::composer3::QtiLayerFlags;
using aidl::vendor::qti::hardware::display::composer3::QtiLayerType;

namespace android::Hwc2 {

class QtiAidlCommandWriter : public ComposerClientWriter {
public:
    explicit QtiAidlCommandWriter(int64_t display)
          : ComposerClientWriter(display), mDisplay(display) {
        qtiReset();
    }
    ~QtiAidlCommandWriter() override { qtiReset(); }

    QtiAidlCommandWriter(QtiAidlCommandWriter&&) = default;
    QtiAidlCommandWriter(const QtiAidlCommandWriter&) = delete;
    QtiAidlCommandWriter& operator=(const QtiAidlCommandWriter&) = delete;

    void qtiSetLayerType(int64_t display, int64_t layer, uint32_t type) {
        auto qtiLayerCommand = qtiGetLayerCommand(display, layer);
        if (qtiLayerCommand) {
            qtiLayerCommand->qtiLayerType = static_cast<QtiLayerType>(type);
        }
    }

    void qtiSetDisplayElapseTime(int64_t display, uint64_t time) {
        auto qtiDisplayCommand = qtiGetDisplayCommand(display);
        if (qtiDisplayCommand) {
            qtiDisplayCommand->time = static_cast<int64_t>(time);
        }
    }

    void qtiSetClientTarget_3_1(int64_t display, uint32_t slot, int acquireFence,
                                uint32_t dataspace) {
        auto qtiDisplayCommand = qtiGetDisplayCommand(display);
        ClientTarget clientTargetCommand;
        clientTargetCommand.buffer = qtiGetBuffer(slot, nullptr, acquireFence);
        clientTargetCommand.dataspace =
                static_cast<aidl::android::hardware::graphics::common::Dataspace>(dataspace);
        if (qtiDisplayCommand) {
            qtiDisplayCommand->clientTarget_3_1.emplace(std::move(clientTargetCommand));
        }
    }

    void qtiSetLayerFlag(int64_t display, int64_t layer, QtiLayerFlags flags) {
        auto qtiLayerCommand = qtiGetLayerCommand(display, layer);
        if (qtiLayerCommand) {
            qtiLayerCommand->qtiLayerFlags = flags;
        }
    }

    void qtiReset() {
        mQtiDisplayCommand.reset();
        mQtiLayerCommand.reset();
        mQtiCommands.clear();
    }

    const std::vector<QtiDisplayCommand>& getPendingQtiCommands() {
        qtiFlushLayerCommand();
        qtiFlushDisplayCommand();
        return mQtiCommands;
    }

private:
    std::optional<QtiDisplayCommand> mQtiDisplayCommand;
    std::optional<QtiLayerCommand> mQtiLayerCommand;
    std::vector<QtiDisplayCommand> mQtiCommands;
    const int64_t mDisplay;

    void qtiFlushLayerCommand() {
        if (mQtiLayerCommand.has_value()) {
            mQtiDisplayCommand->qtiLayers.emplace_back(std::move(*mQtiLayerCommand));
            mQtiLayerCommand.reset();
        }
    }

    void qtiFlushDisplayCommand() {
        if (mQtiDisplayCommand.has_value()) {
            mQtiCommands.emplace_back(std::move(*mQtiDisplayCommand));
            mQtiDisplayCommand.reset();
        }
    }

    Buffer qtiGetBuffer(uint32_t slot, const native_handle_t* bufferHandle, int fence) {
        Buffer bufferCommand;
        bufferCommand.slot = static_cast<int32_t>(slot);
        if (bufferHandle) bufferCommand.handle.emplace(::android::dupToAidl(bufferHandle));
        if (fence > 0) bufferCommand.fence = ::ndk::ScopedFileDescriptor(fence);
        return bufferCommand;
    }

    QtiDisplayCommand* qtiGetDisplayCommand(int64_t display) {
        if (!mQtiDisplayCommand.has_value() ||
            (mQtiDisplayCommand.has_value() && mQtiDisplayCommand->display != display)) {
            if (display != mDisplay) {
                ALOGW("Incorrect display id, skipping QTI command");
                return nullptr;
            }
            qtiFlushLayerCommand();
            qtiFlushDisplayCommand();
            mQtiDisplayCommand.emplace();
            mQtiDisplayCommand->display = display;
        }
        return &(*mQtiDisplayCommand);
    }

    QtiLayerCommand* qtiGetLayerCommand(int64_t display, int64_t layer) {
        auto qtiDisplayCommand = qtiGetDisplayCommand(display);
        if (!qtiDisplayCommand) return nullptr;

        if (!mQtiLayerCommand.has_value() ||
            (mQtiLayerCommand.has_value() && mQtiLayerCommand->layer != layer)) {
            qtiFlushLayerCommand();
            mQtiLayerCommand.emplace();
            mQtiLayerCommand->layer = layer;
        }
        return &(*mQtiLayerCommand);
    }
};
} // namespace android::Hwc2
#endif
