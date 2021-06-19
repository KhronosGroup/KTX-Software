// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2011-2021 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

/**
 * @brief Functions for building the implementation of stb_image and tinyexr.
 */

#include <cstdlib>
#include <cstdio>

// Configure the STB image imagewrite library build.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_PSD

// Configure the TinyEXR library build.
#define TINYEXR_IMPLEMENTATION

// For both libraries force asserts (which can be triggered by corrupt input
// images) to be handled at runtime in release builds to avoid security issues.
#define STBI_ASSERT(x) astcenc_runtime_assert(x)
#define TEXR_ASSERT(x) astcenc_runtime_assert(x)

/**
 * @brief Trap image load failures and convert into a runtime error.
 */
static void astcenc_runtime_assert(bool condition)
{
    if (!condition)
    {
        printf("ERROR: Corrupt input image\n");
        exit(1);
    }
}

#include "stb_image.h"
#include "stb_image_write.h"
#include "tinyexr.h"
