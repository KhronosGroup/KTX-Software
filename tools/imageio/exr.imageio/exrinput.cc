// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// TEXR is not defined in tinyexr.h. Current GitHub tinyexr master uses
// assert. The version in astc-encoder must be old.
#define TEXR_ASSERT(x) assert(x)
#define TINYEXR_IMPLEMENTATION
#include <cassert>
#include "tinyexr.h"
#include "imageio.h"

//#include <cstdlib>
//#include <iterator>
//#include <string>

//
// Documentation on the OpenEXR format can be found at:
// https://openexr.readthedocs.io/en/latest/
// More information about OpenEXR including sample images can be found at:
// https://www.openexr.com/
//

class ExrInput final : public ImageInput {
public:
    ExrInput() : ImageInput("exr") { }
    virtual void open(ImageSpec&) override;

    virtual void readImage(void* /*buffer*/, size_t /*bufferByteCount*/,
                           uint32_t /*subimage = 0*/, uint32_t /*miplevel = 0*/,
                           const FormatDescriptor& /*targetFormat = FormatDescriptor()*/) override { };
    /// Read a single scanline (all channels) of native data into contiguous
    /// memory.
    virtual void readNativeScanline(void* /*buffer*/, size_t /*bufferByteCount*/,
                                    uint32_t /*y*/, uint32_t /*z*/,
                                    uint32_t /*subimage*/, uint32_t /*miplevel*/) override { };

private:
    std::streampos m_header_end_pos;  // file position after the header
    std::string m_current_line;       ///< Buffer the image pixels
    const char* m_pos;
    unsigned int m_max_val;

    bool read_file_scanline(void* data, int y);
    bool read_file_header();
};

ImageInput*
exrInputCreate()
{
    return new ExrInput;
}

const char* exrInputExtensions[] = { "exr", nullptr };

void
ExrInput::open(ImageSpec&)
{
    throw std::runtime_error("Not an EXR file");
}
