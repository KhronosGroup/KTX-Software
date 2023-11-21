// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file imageoutput.cc
//!
//! @brief ImageOutput class implementation
//!

#include "imageio.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <stdarg.h>

std::unique_ptr<ImageOutput>
ImageOutput::create(const std::string& filename)
{
    std::unique_ptr<ImageOutput> out;
    if (filename.empty()) {  // Can't even guess if no name given
        throw std::runtime_error(
            "ImageOutput::create() called with no filename"
        );
    }

    // Populate outputFormats.
    if (Imageio::outputFormats.empty()) {
        Imageio::catalogBuiltinPlugins();
        assert(!Imageio::outputFormats.empty()
               && "No image output plugins compiled in.");
    }

    // Extract the file extension from the filename (without the leading dot)
    Imageio::string format = filename.substr(filename.find_last_of('.')+1);
    if (format.empty()) {
        // If the file had no extension, maybe it was itself the format name
        format = filename;
    }
    format.tolower();

    ImageOutput::Creator createFunction = nullptr;
    // See if it's already in the table.  If not, scan all plugins we can
    // find to populate the table.
    Imageio::OutputPluginMap::const_iterator found
            = Imageio::outputFormats.find(format);
    if (found != Imageio::outputFormats.end()) {
        createFunction = found->second;
    } else {
        throw std::runtime_error(
            fmt::format("Could not find a format writer for \"{}\". "
                    "Is it a file format that we don't know about?",
                    filename));
    }

    assert(createFunction != nullptr);
    out = std::unique_ptr<ImageOutput>(createFunction());
    return out;
}

void
ImageOutput::writeScanline(int /*y*/, int /*z*/,
                           const FormatDescriptor& /*format*/,
                           const void* /*data*/, stride_t /*xstride*/)
{

}

void
ImageOutput::writeImage(const FormatDescriptor& /*format*/,
                        const void* /*data*/,
                        stride_t /*xstride*/,
                        stride_t /*ystride*/,
                        stride_t /*zstride*/,
                        ProgressCallback /*progress_callback*/,
                        void* /*progress_callback_data*/)
{

}

void
ImageOutput::copyImage(ImageInput* /*in*/)
{

}
