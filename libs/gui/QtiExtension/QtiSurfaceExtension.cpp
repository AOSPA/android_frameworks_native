/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0

#include <cutils/properties.h>
#include <regex>
#include <binder/IServiceManager.h>
#include <log/log.h>
#include <gralloctypes/Gralloc4.h>
#include <hidl/HidlSupport.h>

#include <dlfcn.h>
#include <vndksupport/linker.h>
#include <android/hardware/graphics/mapper/IMapper.h>

#include "QtiGralloc.h"
#include "QtiSurfaceExtension.h"

#define SNAP_TYPE_BUFFER_DEQUEUE_DURATION 10045

using android::hardware::graphics::mapper::V4_0::IMapper;
using android::hardware::graphics::mapper::V4_0::Error;
using android::hardware::hidl_vec;
using qtigralloc::MetadataType_BufferDequeueDuration;

namespace android::libguiextension {

typedef AIMapper_Error (*AIMapper_loadIMapperFn)(AIMapper *_Nullable *_Nonnull outImplementation);
constexpr const char *_Nonnull VENDOR_QTI_METADATA_NAME = "QTI";
static AIMapper *sMapper5 = nullptr;

void QtiSurfaceExtension::LoadQtiMapper5() {
    if (!sMapper5) {
        std::string lib_name = "mapper.qti.so";
        void *so = android_load_sphal_library(lib_name.c_str(), RTLD_LOCAL | RTLD_NOW);
        if (!so) {
            ALOGW("Failed to load %s", lib_name.c_str());
            return;
        }

        auto loadIMapper = (AIMapper_loadIMapperFn)dlsym(so, "AIMapper_loadIMapper");
        AIMapper_Error error = loadIMapper(&sMapper5);
        if (error != AIMAPPER_ERROR_NONE) {
            ALOGW("AIMapper_loadIMapper failed %d", error);
        }
    }
}

void QtiSurfaceExtension::InitializeMapper() {
    if (!sMapper5) {
        LoadQtiMapper5();
    }

    if(!sMapper5 && !mMapper4) {
        mMapper4 = IMapper::getService();
        if (mMapper4 == nullptr) {
            ALOGI("mapper 4.x is not supported");
        } else if (mMapper4->isRemote()) {
            LOG_ALWAYS_FATAL("gralloc-mapper must be in passthrough mode");
        }
    }
}

QtiSurfaceExtension::QtiSurfaceExtension(Surface* surface) {
    if (!surface) {
        ALOGW("Invalid pointer to Surface passed");
    } else {
        char value[PROPERTY_VALUE_MAX];
        int int_value = 0;

        property_get("vendor.display.enable_optimal_refresh_rate", value, "0");
        int_value = atoi(value);
        mEnableOptimalRefreshRate = (int_value == 1) ? true : false;
        ALOGV("Successfully created QtiSurfaceExtension");
    }
}

void QtiSurfaceExtension::qtiSetBufferDequeueDuration(std::string layerName,
                                                      android_native_buffer_t* buffer,
                                                      nsecs_t dequeue_duration) {
    if (!buffer) {
        ALOGW("Pointer to android_native_buffer_t is invalid");
        return;
    }

    if (!mEnableOptimalRefreshRate || !isGame(layerName)) {
        return;
    }

    if (sMapper5) {
        auto error = sMapper5->v5.setMetadata(
            buffer->handle,
            {VENDOR_QTI_METADATA_NAME, (int64_t)SNAP_TYPE_BUFFER_DEQUEUE_DURATION},
            static_cast<void *>(&dequeue_duration), sizeof(dequeue_duration));
        if (error != AIMAPPER_ERROR_NONE) {
            ALOGW("setMetadata (%d) failed, buffer size %zd error:%d",
                  static_cast<int64_t>(SNAP_TYPE_BUFFER_DEQUEUE_DURATION),
                  sizeof(dequeue_duration), error);
        }
    } else if (mMapper4) {
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
            mMapper4->set(const_cast<native_handle_t*>(buffer->handle),
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
            // It is not an error to attempt to set metadata that a particular
            // gralloc implementation happens to not support.
            case Error::UNSUPPORTED:
            case Error::NONE:
                break;
        }
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
            InitializeMapper();
            return true;
        }
    }

    return false;
}

} // namespace android::libguiextension
