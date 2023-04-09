// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "stdafx.h"
#include "validation_messages.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

struct ValidationReport {
    IssueType type;
    uint16_t id;
    std::string message;
    std::string details;
};

int validateIOStream(std::istream& stream, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback);
int validateMemory(const char* data, std::size_t size, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback);
int validateNamedFile(const _tstring& filepath, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback);
int validateStdioStream(FILE* file, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback);

} // namespace ktx
