// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2010-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file image.cc
//!
//! @brief Image creation functions
//!

#include "stdafx.h"

#include <sstream>
#include <stdexcept>
#include <vector>

#include "image.hpp"

const std::vector<Image::CreateFunction> Image::CreateFunctions = {
    CreateFromNPBM,
    CreateFromPNG,
    CreateFromJPG
};

Image* Image::CreateFromFile(const _tstring& name,
                             bool transformOETF, bool rescaleTo8Bit) {
    FILE* f;
    Image* image;

    f = _tfopen(name.c_str(), "rb");
    if (!f) {
      std::stringstream message;

      message << "Could not open input file \"" << name << "\". ";
      message << strerror(errno);
      throw std::runtime_error(message.str());
    }

    std::vector<CreateFunction>::const_iterator
        func = CreateFunctions.begin();
    for (; func < CreateFunctions.end(); func++ ) {
        try {
            image = (*func)(f, transformOETF, rescaleTo8Bit);
            return image;
        } catch (different_format) {
            rewind(f);
            continue;
        }
    }
    if (func == CreateFunctions.end()) {
        std::stringstream message;

        message << "Format of input file \"" << name << "\" is unsupported.";
        throw std::runtime_error(message.str());
    }
    return nullptr; // Keep compilers happy.
}

