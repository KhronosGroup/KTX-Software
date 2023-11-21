// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

/**
 * @internal
 * @~English
 * @file pngoutput.cc
 *
 * @brief ImageOutput to  PNG format files.
 *
 * @author Mark Callow
 */

#include "imageio.h"

#include <iterator>
#include <sstream>
#include <stdexcept>

#include "lodepng.h"
#include <KHR/khr_df.h>
#include "dfd.h"

class pngOutput final : public ImageOutput {
  public:
    pngOutput() : ImageOutput("png") {}
    virtual ~pngOutput() { close(); }
    virtual void close() override { }
    virtual void open(const std::string& name, const ImageSpec& newspec,
                      OpenMode mode) override;

  protected:
};

ImageOutput*
pngOutputCreate()
{
    return new pngOutput;
}

const char* pngOutputExtensions[] = { "png", nullptr };

void
pngOutput::open(const std::string& /*name*/, const ImageSpec& /*newspec*/,
                OpenMode /*mode*/)
{

}
