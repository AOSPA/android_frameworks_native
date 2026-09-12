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
#include <include/private/SkHdrMetadata.h>
#include <ui/GraphicTypes.h>

#include <optional>
#include "SkColorSpace.h"

namespace android {
namespace renderengine {

enum class ColorSpaceOptions : uint32_t {
    None = 0,
    USE_HLG_OOTF = 1 << 0,
};

// Converts an android dataspace to a supported SkColorSpace
// Supported dataspaces are
// 1. sRGB
// 2. Display P3
// 3. BT2020 PQ
// 4. BT2020 HLG
// Unknown primaries are mapped to BT709, and unknown transfer functions
// are mapped to sRGB.
sk_sp<SkColorSpace> toSkColorSpace(ui::Dataspace dataspace,
                                   ftl::Flags<ColorSpaceOptions> options = ColorSpaceOptions::None);

/**
 * Returns the maximum headroom allowed for this content based on the
 * SMPTE 2094-50 metadata.
 */
inline float getMaxHeadroom(const std::optional<skhdr::AdaptiveGlobalToneMap>& agtm) {
    if (agtm && agtm->fHeadroomAdaptiveToneMap) {
        float maxAgtmRatio = agtm->fHeadroomAdaptiveToneMap->fBaselineHdrHeadroom;
        for (const auto& alternativeImage : agtm->fHeadroomAdaptiveToneMap->fAlternateImages) {
            maxAgtmRatio = std::max(maxAgtmRatio, alternativeImage.fHdrHeadroom);
        }
        return std::pow(2.f, maxAgtmRatio);
    }
    return 1.f;
}

} // namespace renderengine
} // namespace android