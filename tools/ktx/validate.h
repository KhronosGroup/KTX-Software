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
