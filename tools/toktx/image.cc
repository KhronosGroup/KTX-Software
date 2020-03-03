// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

//
// ©2010 The Khronos Group, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

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
    CreateFromPNG
};

Image* Image::CreateFromFile(_tstring& name, bool transformOETF) {
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
            image = (*func)(f, transformOETF);
            return image;
        } catch (different_format) {
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


