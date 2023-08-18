/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0

#include <cutils/properties.h>
#include <regex>
#include <binder/IServiceManager.h>
#include <log/log.h>
#include <gralloctypes/Gralloc4.h>
#include <hidl/HidlSupport.h>

#include "QtiGralloc.h"
#include "QtiSurfaceExtension.h"

using android::hardware::graphics::mapper::V4_0::IMapper;
using android::hardware::graphics::mapper::V4_0::Error;
using android::hardware::hidl_vec;
using qtigralloc::MetadataType_BufferDequeueDuration;

namespace android::libguiextension {

QtiSurfaceExtension::QtiSurfaceExtension(Surface* surface)
      : mQtiSurface(surface) {
    if (!mQtiSurface) {
        ALOGW("Invalid pointer to Surface passed");
    } else {
        mMapper = IMapper::getService();
        if (mMapper == nullptr) {
            ALOGI("mapper 4.x is not supported");
        } else if (mMapper->isRemote()) {
            LOG_ALWAYS_FATAL("gralloc-mapper must be in passthrough mode");
        }

        char value[PROPERTY_VALUE_MAX];
        int int_value = 0;

        property_get("vendor.display.enable_optimal_refresh_rate", value, "0");
        int_value = atoi(value);
        enable_optimal_refresh_rate_ = (int_value == 1) ? true : false;
        ALOGV("Successfully created QtiSurfaceExtension");
    }
}

void QtiSurfaceExtension::qtiSetBufferDequeueDuration(std::string layerName,
                                                      android_native_buffer_t* buffer,
                                                      nsecs_t dequeue_duration) {
    if (!mMapper) {
        return;
    }

    if (!buffer) {
        ALOGW("Pointer to android_native_buffer_t is invalid");
        return;
    }

    if (!enable_optimal_refresh_rate_ || !isGame(layerName)) {
        return;
    }

    hidl_vec<uint8_t> encodedMetadata;
    const status_t status =
        gralloc4::encodeInt64(MetadataType_BufferDequeueDuration,
                              dequeue_duration, &encodedMetadata);
    if (status != OK) {
        ALOGE("Encoding metadata(%s) failed with %d",
              MetadataType_BufferDequeueDuration.name.c_str(), status);
        return;
    }

    auto ret =
        mMapper->set(const_cast<native_handle_t*>(buffer->handle),
                     MetadataType_BufferDequeueDuration, encodedMetadata);
    const Error error = ret.withDefault(Error::NO_RESOURCES);
    switch (error) {
        case Error::BAD_DESCRIPTOR:
        case Error::BAD_BUFFER:
        case Error::BAD_VALUE:
        case Error::NO_RESOURCES:
            ALOGE("set(%s, %" PRIu64 ", ...) failed with %d",
                  MetadataType_BufferDequeueDuration.name.c_str(),
                  MetadataType_BufferDequeueDuration.value, error);
            break;
        // It is not an error to attempt to set metadata that a particular gralloc implementation
        // happens to not support.
        case Error::UNSUPPORTED:
        case Error::NONE:
            break;
    }
}

bool QtiSurfaceExtension::isGame(std::string layerName) {
    if (layerName == mQtiLayerName) {
        return mQtiIsGame;
    }

    mQtiLayerName = layerName;
    mQtiIsGame = false;
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> perfservice = sm->checkService(String16("vendor.perfservice"));
    if (perfservice == nullptr) {
        ALOGE("Cannot find perfservice");
        return false;
    }
    String16 ifName = perfservice->getInterfaceDescriptor();
    if (ifName.size() > 0) {
        const std::regex re("(?:SurfaceView\\[)([^/]+).*");
        std::smatch match;
        if (!std::regex_match(layerName, match, re)) {
            return false;
        }
        String16 pkgName = String16(match[1].str().c_str());

        Parcel data, reply;
        int GAME_TYPE = 2;
        int VENDOR_FEEDBACK_WORKLOAD_TYPE = 0x00001601;
        int PERF_GET_FEEDBACK = IBinder::FIRST_CALL_TRANSACTION + 7;
        int array[0];
        data.markForBinder(perfservice);
        data.writeInterfaceToken(ifName);
        data.writeInt32(VENDOR_FEEDBACK_WORKLOAD_TYPE);
        data.writeString16(pkgName);
        data.writeInt32(getpid());
        data.writeInt32Array(0, array);
        perfservice->transact(PERF_GET_FEEDBACK, data, &reply);
        reply.readExceptionCode();
        int type = reply.readInt32();
        if (type == GAME_TYPE) {
            mQtiIsGame = true;
            return true;
        }
    }

    return false;
}


} // namespace android::libguiextension
