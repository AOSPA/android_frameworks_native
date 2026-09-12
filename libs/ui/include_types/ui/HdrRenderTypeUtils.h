/*
 * Copyright 2021 The Android Open Source Project
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

#pragma once

#include <ftl/flags.h>
#include <ui/GraphicTypes.h>

#include <cmath>
#include <optional>

namespace android {

enum class HdrMetadataOptions : uint32_t {
    None = 0,
    HasHdrMetadata = 1 << 0,
    HasSmpte2094_50 = 1 << 1,
};

enum class HdrRenderType {
    SDR,         // just render to SDR
    DISPLAY_HDR, // HDR by extended brightness
    GENERIC_HDR  // tonemapped HDR
};

/***
 * A helper function to classify how we treat the result based on params.
 *
 * @param dataspace the dataspace
 * @param pixelFormat optional, in case there is no source buffer.
 * @param hdrSdrRatio default is 1.f, render engine side doesn't take care of it.
 * @return HdrRenderType
 */
inline HdrRenderType getHdrRenderType(
        ui::Dataspace dataspace, std::optional<ui::PixelFormat> pixelFormat,
        float hdrSdrRatio = 1.f,
        ftl::Flags<HdrMetadataOptions> options = HdrMetadataOptions::None) {
    if (options.test(HdrMetadataOptions::HasSmpte2094_50)) {
        return HdrRenderType::GENERIC_HDR;
    }
    const auto transfer = dataspace & HAL_DATASPACE_TRANSFER_MASK;
    const auto range = dataspace & HAL_DATASPACE_RANGE_MASK;

    if (transfer == HAL_DATASPACE_TRANSFER_ST2084 || transfer == HAL_DATASPACE_TRANSFER_HLG) {
        return HdrRenderType::GENERIC_HDR;
    }

    static const auto BT2020_LINEAR_EXT = static_cast<ui::Dataspace>(HAL_DATASPACE_STANDARD_BT2020 |
                                                                     HAL_DATASPACE_TRANSFER_LINEAR |
                                                                     HAL_DATASPACE_RANGE_EXTENDED);

    if ((dataspace == BT2020_LINEAR_EXT || dataspace == ui::Dataspace::V0_SCRGB) &&
        pixelFormat.has_value() && pixelFormat.value() == ui::PixelFormat::RGBA_FP16 &&
        options.test(HdrMetadataOptions::HasHdrMetadata)) {
        return HdrRenderType::GENERIC_HDR;
    }

    // Extended range layer with an hdr/sdr ratio of > 1.01f can "self-promote" to HDR.
    if (range == HAL_DATASPACE_RANGE_EXTENDED && hdrSdrRatio > 1.01f) {
        return HdrRenderType::DISPLAY_HDR;
    }

    return HdrRenderType::SDR;
}

/**
 * Returns the scale factor for HLG and PQ content relative to SDR.
 * The scale factor is the value of SDR white, inside of the signal
 * whose transfer is described by the dataspace, normalized on a range from 0 to 1.
 * I.e., by how much do we scale an HDR signal to fit within 0 to 1.
 *
 * For PQ, the scale factor would be 203 / 10,000.
 * For HLG, the scale factor would be approximately 265 / 1,000
 * This aligns with SDR white corresponding to 203 nits inside of an HDR signal, with
 * the assumption that HLG will later apply a nonlinear OOTF with gamma = 1.2 (so that 265 nits maps
 * to 203 nits after the OOTF)
 *
 * @param dataspace the dataspace
 * @return the scale factor
 */
inline float getSdrRelativeScaleFactor(ui::Dataspace dataspace) {
    const auto transfer = dataspace & HAL_DATASPACE_TRANSFER_MASK;
    switch (transfer) {
        case HAL_DATASPACE_TRANSFER_HLG: {
            static constexpr float input = 0.7498773651f;
            static constexpr float a = 0.17883277f;
            static constexpr float b = 1.f - 4.f * a;
            const static float c = 0.5f - a * std::log(4.f * a);
            return (std::exp((input - c) / a) + b) / 12.f;
        }
        case HAL_DATASPACE_TRANSFER_ST2084: {
            static constexpr float input = 0.58068888104f;
            static constexpr float m1 = 0.1593017578125f;
            static constexpr float m2 = 78.84375f;
            static constexpr float c1 = 0.8359375f;
            static constexpr float c2 = 18.8515625f;
            static constexpr float c3 = 18.6875f;
            return std::pow((std::pow(input, 1.f / m2) - c1) /
                                    (c2 - c3 * std::pow(input, 1.f / m2)),
                            1.f / m1);
        }
        default:
            return 1.0f;
    }
}

/**
 * Returns the maximum headroom allowed for this content under "idealized"
 * display conditions (low surround luminance, high-enough display brightness).
 *
 * TODO: take into account hdr metadata, but square it with the fact that some
 * HLG content has CTA.861-3 metadata
 */
inline float getIdealizedMaxHeadroom(ui::Dataspace dataspace, float agtmMaxHeadroom = -1.f) {
    if (agtmMaxHeadroom >= 1.f) {
        return agtmMaxHeadroom;
    }
    const auto transfer = dataspace & HAL_DATASPACE_TRANSFER_MASK;

    switch (transfer) {
        case HAL_DATASPACE_TRANSFER_ST2084:
            return 10000.0f / 203.0f;
        case HAL_DATASPACE_TRANSFER_HLG:
            return 1000.0f / 203.0f;
        default:
            return 1.0f;
    }
}

} // namespace android
