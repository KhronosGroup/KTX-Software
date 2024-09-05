// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "validation_messages.h"
#include "command.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

static constexpr uint32_t MAX_NUM_DFD_BLOCKS = 10;
static constexpr uint32_t MAX_NUM_BDFD_SAMPLES = 16;
static constexpr uint32_t MAX_NUM_KV_ENTRIES = 100;

struct DFDHeader {
    uint32_t vendorId: 17;
    uint32_t descriptorType: 15;
    uint32_t versionNumber: 16;
    uint32_t descriptorBlockSize: 16;
};
static_assert(sizeof(DFDHeader) == 8);

struct BDFD {
    uint32_t vendorId: 17;
    uint32_t descriptorType: 15;
    uint32_t versionNumber: 16;
    uint32_t descriptorBlockSize: 16;
    uint32_t model: 8;
    uint32_t primaries: 8;
    uint32_t transfer: 8;
    uint32_t flags: 8;
    uint32_t texelBlockDimension0: 8;
    uint32_t texelBlockDimension1: 8;
    uint32_t texelBlockDimension2: 8;
    uint32_t texelBlockDimension3: 8;
    std::array<uint8_t, 8> bytesPlanes;

    [[nodiscard]] bool matchTexelBlockDimensions(uint8_t dim0, uint8_t dim1, uint8_t dim2, uint8_t dim3) const {
        return texelBlockDimension0 == dim0
                && texelBlockDimension1 == dim1
                && texelBlockDimension2 == dim2
                && texelBlockDimension3 == dim3;
    }

    [[nodiscard]] bool hasNonZeroBytePlane() const {
        return bytesPlanes[0] != 0 || bytesPlanes[1] != 0
                || bytesPlanes[2] != 0 || bytesPlanes[3] != 0
                || bytesPlanes[4] != 0 || bytesPlanes[5] != 0
                || bytesPlanes[6] != 0 || bytesPlanes[7] != 0;
    }
};
static_assert(sizeof(BDFD) == 24);

struct SampleType {
    uint32_t bitOffset: 16;
    uint32_t bitLength: 8;
    uint32_t channelType: 4;
    uint32_t qualifierLinear: 1;
    uint32_t qualifierExponent: 1;
    uint32_t qualifierSigned: 1;
    uint32_t qualifierFloat: 1;
    uint32_t samplePosition0: 8;
    uint32_t samplePosition1: 8;
    uint32_t samplePosition2: 8;
    uint32_t samplePosition3: 8;
    uint32_t lower;
    uint32_t upper;
};
static_assert(sizeof(SampleType) == 16);

struct ValidationReport {
    IssueType type;
    uint16_t id;
    std::string message;
    std::string details;
};

/// Common function for tools to validates the input file (and rewind the stream)
/// @param stream the stream to be validated
/// @param inputFilepath only used for logging
/// @param report
/// @throw FatalError if there was any error or the file is considered invalid
void validateToolInput(std::istream& stream, const std::string& inputFilepath, Reporter& report);

int validateIOStream(std::istream& stream, const std::string& filepath, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback);
int validateMemory(const char* data, std::size_t size, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback);
int validateNamedFile(const std::string& filepath, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback);
int validateStdioStream(FILE* file, const std::string& filepath, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback);

} // namespace ktx
