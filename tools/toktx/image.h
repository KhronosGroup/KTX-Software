// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// $Id$

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
//! @file image.h
//!
//! @brief Internal interface for netpbm file reader
//!

#ifndef IMAGE_H
#define IMAGE_H

enum FileResult { SUCCESS, INVALID_FORMAT, INVALID_VALUE, INVALID_PAM_HEADER,
                  INVALID_TUPLETYPE, UNEXPECTED_EOF, IO_ERROR, OUT_OF_MEMORY };

FileResult readNPBM(FILE* src, unsigned int& width, unsigned int& height,
                    unsigned int& components, unsigned int& componentSize,
                    unsigned int& imageSize, unsigned char** pixels);

FileResult readPAM(FILE* src, unsigned int& width, unsigned int& height,
                    unsigned int& components, unsigned int& componentSize,
                    unsigned int& imageSize, unsigned char** pixels);

FileResult readPPM(FILE* src, unsigned int& width, unsigned int& height,
                    unsigned int& components, unsigned int& componentSize,
                    unsigned int& imageSize, unsigned char** pixels);

FileResult readPGM(FILE* src, unsigned int& width, unsigned int& height,
                    unsigned int& components, unsigned int& componentSize,
                    unsigned int& imageSize, unsigned char** pixels);

FileResult readImage(FILE* src, unsigned int imageSize, unsigned char*& pixels);

#endif



