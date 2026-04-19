// Copyright 2022-2023 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "utility.h"

// -------------------------------------------------------------------------------------------------

namespace ktx {

auto findMetadataValue(KTXTexture2& texture, const char* const key) {
    const char* value;
    uint32_t valueLen;
    std::string result;
    auto ret = ktxHashList_FindValue(&texture.handle()->kvDataHead, key,
                  &valueLen, (void**)&value);
    if (ret == KTX_SUCCESS) {
        // The values we are looking for are required to be NUL terminated.
        result.assign(value, valueLen - 1);
    }
    return result;
};

auto findFloatMetadataValue(KTXTexture2& texture, const char* const key) {
    const float* value;
    uint32_t valueLen;
    std::vector<float> result;
    auto ret = ktxHashList_FindValue(&texture.handle()->kvDataHead, key,
                  &valueLen, (void**)&value);
    if (ret == KTX_SUCCESS) {
        uint32_t numFloats = valueLen / sizeof(float);
        result.resize(numFloats);
        std::memcpy(result.data(), value, valueLen);
    }
    return result;
};

auto updateMetadataValue(KTXTexture2& texture, const char* const key,
                               const std::string& value) {
    ktxHashList_DeleteKVPair(&texture.handle()->kvDataHead, key);
    ktxHashList_AddKVPair(&texture.handle()->kvDataHead, key,
            static_cast<uint32_t>(value.size() + 1), // +1 to include \0
            value.c_str());
};

} // namespace ktx
