// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2010-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file
//!
//! @brief Create Images from JPEG format files.
//!
//! @author Mark Callow, HI Corporation.
//! @author Jacob Str&ouml;m, Ericsson AB.
//!

#include "stdafx.h"

#include <sstream>
#include <stdexcept>

#include "image.hpp"
#include "encoder/jpgd.h"

using namespace jpgd;

class myjpgdstream : public jpeg_decoder_file_stream {
  public:
    myjpgdstream(FILE* stdioStream) {
        m_pFile = stdioStream;
        m_eof_flag = false;
        m_error_flag = false;
    }

    int read(uint8_t* pBuf, int max_bytes_to_read, bool* pEOF_flag) {
        if (!m_pFile)
          return -1;

        if (m_eof_flag)
        {
          *pEOF_flag = true;
          return 0;
        }

        if (m_error_flag)
          return -1;

        size_t bytes_read = fread(pBuf, 1, max_bytes_to_read, m_pFile);
        if (bytes_read < max_bytes_to_read)
        {
          if (ferror(m_pFile))
          {
            m_error_flag = true;
            return -1;
          }

          m_eof_flag = true;
          *pEOF_flag = true;
        }

        return static_cast<int>(bytes_read);
    }

    void rewind() {
        ::rewind(m_pFile);
        m_eof_flag = false;
    }

  protected:
   FILE*  m_pFile;
   bool m_eof_flag, m_error_flag;
};

// All JPEG files are sRGB.
Image*
Image::CreateFromJPG(FILE* src, bool, bool)
{
    myjpgdstream stream(src);
    uint32_t componentCount;

    // Figure out how many components so we can request that number from
    // the decoder.
    {
        jpeg_decoder jd(&stream, jpeg_decoder::cFlagLinearChromaFiltering);
        jpgd_status errorCode = jd.get_error_code();

        if (errorCode != JPGD_SUCCESS) {
            if (errorCode == JPGD_NOT_JPEG) {
                throw Image::different_format();
            } else if (errorCode == JPGD_NOTENOUGHMEM) {
                throw std::runtime_error("JPEG decoder out of memory");
            } else {
                std::stringstream message;
                message << "Invalid data in JPEG file. jpgd_status code: "
                        << errorCode;
                throw std::runtime_error(message.str());
            }
        }
        componentCount = jd.get_num_components();
    }
    // The decoder is now closed. We don't use it because it's a per scan line
    // thing so needs a lot of code, which is in the following function.

    stream.rewind();

    int w = 0, h = 0, actual_comps = 0;
    if (componentCount == 4) {
        // It's most likely an Adobe created YCCK image with the 4th
        // component used to recreate the original CMYK image. We
        // can probably safely ignore it so we'll request just 3 components.
        --componentCount;
    }
    uint8_t* imageData = decompress_jpeg_image_from_stream(&stream,
                              &w, &h, &actual_comps, componentCount,
                              jpeg_decoder::cFlagLinearChromaFiltering);

    if (imageData == NULL) {
         throw std::runtime_error("JPEG decode failed");
    }

    Image* image;
    switch (componentCount) {
      case 1: {
        image = new r8image(w, h, (r8color*)imageData);
        image->colortype = Image::eLuminance;
        break;
      } case 3: {
        image = new rgb8image(w, h, (rgb8color*)imageData);
        image->colortype = Image::eRGB;
        break;
      }
    }

    // All JPEG images are sRGB.
    image->setOetf(KHR_DF_TRANSFER_SRGB);
    return image;
}

