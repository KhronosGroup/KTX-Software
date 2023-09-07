// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file imageio.cc
//!
//! @brief Create plugin maps.
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

#define PLUGENTRY(name)                          \
    ImageInput* name##InputCreate();             \
    ImageOutput* name##OutputCreate();           \
    extern const char* name##InputExtensions[];  \
    extern const char* name##OutputExtensions[];
#define PLUGENTRY_RO(name)                       \
    ImageInput* name##InputCreate();             \
    extern const char* name##InputExtensions[];
#define PLUGENTRY_WO(name)                       \
    ImageOutput* name##OutputCreate();           \
    extern const char* name##OutputExtensions[];

PLUGENTRY_RO(exr);
PLUGENTRY_RO(jpeg);
PLUGENTRY_RO(npbm);
PLUGENTRY(png)

namespace Imageio {

// These combine extensions and format names into a single map.
InputPluginMap inputFormats;
OutputPluginMap outputFormats;

void
declareImageioFormat(const std::string& formatname,
                     ImageInput::Creator inputCreator,
                     const char** inputExtensions,
                     ImageOutput::Creator outputCreator,
                     const char** outputExtensions)
{
    if (inputCreator) {
        for (const char** e = inputExtensions; e && *e; ++e) {
            string ext(*e);
            ext.tolower();
            if (inputFormats.find(ext) == inputFormats.end()) {
                inputFormats[ext] = inputCreator;
            }
        }
        if (inputFormats.find(formatname) == inputFormats.end())
            inputFormats[formatname] = inputCreator;
    }
    if (outputCreator) {
        for (const char** e = outputExtensions; e && *e; ++e) {
            string ext(*e);
            ext.tolower();
            if (outputFormats.find(ext) == outputFormats.end()) {
                outputFormats[ext] = outputCreator;
            }
        }
        if (outputFormats.find(formatname) == outputFormats.end())
            outputFormats[formatname] = outputCreator;
    }
}

void
catalogBuiltinPlugins()
{
#define DECLAREPLUG(name)                                      \
        declareImageioFormat(                                  \
            #name, (ImageInput::Creator)name##InputCreate,     \
            name##InputExtensions,                             \
            (ImageOutput::Creator)name##OutputCreate,          \
            name##OutputExtensions)
#define DECLAREPLUG_RO(name)                                   \
        declareImageioFormat(                                  \
            #name, (ImageInput::Creator)name##InputCreate,     \
            name##InputExtensions,                             \
            nullptr, nullptr)
#define DECLAREPLUG_WO(name)                                   \
        declareImageioFormat(                                  \
            nullptr, nullptr,                                  \
            #name, (ImageOutput::Creator)name##OutputCreate,   \
            name##OutputExtensions)

#if !defined(DISABLE_OPENEXR)
    DECLAREPLUG_RO (exr);
#endif
#if !defined(DISABLE_JPEG)
    DECLAREPLUG_RO(jpeg);
#endif
#if !defined(DISABLE_NPBM)
    DECLAREPLUG_RO (npbm);
#endif
#if !defined(DISABLE_PNG)
    DECLAREPLUG (png);
#endif
    // 'Raw' format is not in the catalog as an explicit API is a better fit
}

} // namespace Imageio
