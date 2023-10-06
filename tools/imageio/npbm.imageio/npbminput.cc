// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include "imageio.h"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

/** @internal
 * @~English
 * @file
 *
 * @brief ImageInput from netpbm format files (.pam, .pbm, .pgm or .ppm).
 *
 * Plain formats (magic numbers 'P1', 'P2' & 'P3') are not supported.
 *
 * PPM and PGM specify that sample values are encoded with the BT.709 OETF.
 * They do not indicate that bt.709 only applies when maxval <= 255 so this
 * class always reports OETF as bt.709 for color and grayscale. The
 * specifications also say that both sRGB and linear encoding are often used.
 * Since there is no metadata to indicate a differing OETF this loader always
 * assumes bt.709.
 *
 * Documentation on the netpbm formats can be found at:
 * http://netpbm.sourceforge.net/doc/
 *
 * @author Mark Callow
 */


class NpbmInput final : public ImageInput {
public:
    NpbmInput() : ImageInput("npbm") { }
    virtual void open(ImageSpec& newspec) override;
    virtual void close() override;
    //virtual bool read_native_scanline(uint32_t subimage, uint32_t miplevel, int y, int z,
    //                                  void* data) override;
    virtual void readImage(void* buffer, size_t bufferByteCount,
                           uint32_t subimage, uint32_t miplevel,
                           const FormatDescriptor& targetFormat) override;
    /// Read a single scanline (all channels) of native data into contiguous
    /// memory.
    virtual void readNativeScanline(void* buffer, size_t bufferByteCount,
                                    uint32_t y, uint32_t z,
                                    uint32_t subimage, uint32_t miplevel) override;

    virtual uint32_t subimageCount(void) const override {
        return static_cast<uint32_t>(images.size());
    }
    virtual bool seekSubimage(uint32_t subimage, uint32_t miplevel = 0) override;

    virtual const ImageSpec& spec(void) const override {
        return images[curSubimage].spec;
    }

    using ImageInput::spec;

private:
    std::string currentLine;
    std::istream::iostate exceptionsIn;
    size_t pos;
    unsigned int curImageScanline;
    using ImageInput::rescale;

    void readImageHeaders();
    void parseAHeader();
    enum class filetype {PGM, PPM};
    void parseGPHeader(filetype ftype);
    void nextLine();
    void nextToken();
    void skipComments(char comment = '#');
    void swap(void* pBuffer, size_t nvals);
};


ImageInput*
npbmInputCreate()
{
    return new NpbmInput;
}

const char* npbmInputExtensions[] = { "pam", "pbm", "pgm", "ppm", nullptr };


void
NpbmInput::open(ImageSpec& newspec)
{
    assert(isp != nullptr && "istream not initialized");
    currentLine = "";
    pos         = 0;

    isp->exceptions(std::istream::failbit | std::istream::badbit | std::istream::eofbit );
    readImageHeaders();
    seekSubimage(0);
    newspec = spec();
}


void
NpbmInput::close() {
    isp->exceptions(exceptionsIn);
    ImageInput::close();
}

static int
tupleComponentCount(const char* tupleType)
{
    if (strcmp(tupleType, "BLACKANDWHITE") == 0)
        return 1;
    else if (strcmp(tupleType, "GRAYSCALE") == 0)
        return 1;
    else if (strcmp(tupleType, "GRAYSCALE_ALPHA") == 0)
        return 2;
    else if (strcmp(tupleType, "RGB") == 0)
        return 3;
    else if (strcmp(tupleType, "RGB_ALPHA") == 0)
        return 4;
    else
        return -1;
}


inline void
NpbmInput::nextLine()
{
    std::getline(*isp, currentLine);
    pos = 0;
}


inline void
NpbmInput::nextToken()
{
    while (1) {
        while (isspace(currentLine[pos]))
            pos++;
        if (pos != currentLine.size())
            break;
        else
            nextLine();
    }
}



inline void
NpbmInput::skipComments(char comment)
{
    while (1) {
        nextToken();
        if (currentLine[pos] == comment)
            nextLine();
        else
            break;
    }
}


void
NpbmInput::readImageHeaders()
{
    for(;;) {
        try {
            // MagicNumber
            // If not an NPBM file, there may not be a line terminator.
            currentLine.resize(3);
            isp->read(&currentLine[0], 3);
            if (!currentLine.compare("P7\n")) {
                parseAHeader();
            } else if (!currentLine.compare("P5\n")) {
                parseGPHeader(filetype::PGM);
                //images.back().spec.colortype = colortype_e::Luminance;
            } else if (!currentLine.compare("P6\n")) {
                parseGPHeader(filetype::PPM);
            } else if (!currentLine.compare("P1\n")
                     || !currentLine.compare("P2\n")
                     || !currentLine.compare("P3\n")) {
                throw std::runtime_error("Plain netpbm formats are not supported.");
            } else if (!currentLine.compare("P4\n")) {
                throw std::runtime_error(".pbm files are not supported.");
            } else {
                throw different_format();
            }
            images.back().filepos = isp->tellg(); // Save image data start pos.
            // We've only read the header. Seek to the expected end
            // of the image.
            isp->seekg(images.back().spec.imageByteCount(), isp->cur);
        } catch (const std::istream::failure&) {
            throwOnReadFailure();
        }
        // Check if there is any more data in the file.
        try {
            isp->peek();
        } catch (const std::istream::failure&) {
            if (isp->eof()) {
                return;
            } else {
                throwOnReadFailure();
            }
        }
    }
}

/*
 * SwapEndian16: Swaps endianness in an array of 16-bit values
 */
static void
swapEndian16(uint16_t* pData16, size_t nvals)
{
    for (size_t i = 0; i < nvals; ++i)
    {
        uint16_t x = *pData16;
        *pData16++ = (x << 8) | (x >> 8);
    }
}

static const union foo { uint16_t x; uint8_t c; } bar{1};
#define IS_LITTLE_ENDIAN (bar.c)

bool
littleendian(void) noexcept
{
    return bar.c;
}


void
NpbmInput::swap(void* pBuffer, size_t nvals)
{
    if (spec().format().channelBitLength(KHR_DF_CHANNEL_RGBSDA_R) == 16) {
        // If 2 bytes, MSB is first.
        if (littleendian()) {
            swapEndian16((uint16_t*)pBuffer, nvals);
        }
    }
}


void
NpbmInput::readNativeScanline(void* bufferOut, size_t bufferByteCount,
                              uint32_t y, uint32_t z,
                              uint32_t subimage, uint32_t miplevel)
{
    if (isp == nullptr)
        throw std::runtime_error("istream not initialized");
    if (z > 1)
        throw std::runtime_error("npbm does not support 3d images.");
    if (bufferByteCount < spec().scanlineByteCount())
        throw buffer_too_small();

    seekSubimage(subimage, miplevel);

    if (y != curImageScanline)
        isp->seekg(images[currentSubimage()].filepos + (spec().scanlineByteCount() * y),
                   isp->beg);
    isp->read((char*)bufferOut, spec().scanlineByteCount());
    swap(bufferOut, spec().scanlineByteCount());
}


bool
NpbmInput::seekSubimage(uint32_t subimage, uint32_t miplevel)
{
    if (subimage == currentSubimage() && miplevel == currentMiplevel())
        return true;

    if (subimage >= images.size() || miplevel > 0)
        return false;

    isp->seekg(images[subimage].filepos, isp->beg);
    curSubimage = subimage;
    curMiplevel = miplevel;
    curImageScanline = 0;
    return true;
}


///
/// @internal
/// @~English
/// @brief parse the header of a PGM or PPM file.
///
/// @param [in]  src    pointer to FILE stream to read
/// @param [out] width  reference to a var in which to write the image width.
/// @param [out] height reference to a var in which to write the image height
/// @param [out] maxval reference to a var in which to write the maxval.
///
/// @exception invalid_file if there is no width or height, if maxval is not
///                         an integer or if maxval is out of range.
///
void
NpbmInput::parseAHeader()
{
#define MAX_TUPLETYPE_SIZE 20
#define xtupletype_sscanf_fmt(ms) tupletype_sscanf_fmt(ms)
#define tupletype_sscanf_fmt(ms) "TUPLTYPE %"#ms"s"
    char tupleType[MAX_TUPLETYPE_SIZE+1];   // +1 for terminating NUL.
    uint32_t width = 0, height = 0;
    unsigned int numFieldsFound = 0;
    uint32_t componentCount = 0;
    uint32_t tCompCount = 0;
    uint32_t maxVal = 0;

    for (;;) {
        nextLine();
        skipComments();
        if (currentLine.compare("ENDHDR") == 0)
            break;

        const char* cur_line_cs = currentLine.c_str();
        if (sscanf(cur_line_cs, "HEIGHT %u", &height))
            numFieldsFound++;
        else if (sscanf(cur_line_cs, "WIDTH %u", &width))
            numFieldsFound++;
        else if (sscanf(cur_line_cs, "DEPTH %u", &componentCount))
            numFieldsFound++;
        else if (sscanf(cur_line_cs, "MAXVAL %u", &maxVal))
            numFieldsFound++;
        else if (sscanf(cur_line_cs, xtupletype_sscanf_fmt(MAX_TUPLETYPE_SIZE),
                        tupleType))
            numFieldsFound++;
    };

    if (numFieldsFound < 5) {
        throw invalid_file("Missing fields in pam header.");
    }

    if ((tCompCount = tupleComponentCount(tupleType)) < 1) {
        throw invalid_file(
            fmt::format("Invalid TUPLTYPE: {}.", tupleType)
        );
    }

    if (componentCount < tCompCount) {
        throw invalid_file(
            fmt::format("Mismatched TUPLTYPE, {}, and DEPTH, {}.",
                        tupleType, componentCount)
        );
    }

    if (maxVal <= 0 || maxVal >= (1<<16)) {
        throw std::runtime_error(
            fmt::format("Max color component value must be > 0 && < 65536. "
                        "It is {}", maxVal)
        );
    }

    images.emplace_back(ImageSpec(width, height, 1, componentCount,
                                 maxVal > 255 ? 16 : 8,
                                 0U, maxVal,
                                 static_cast<khr_df_sample_datatype_qualifiers_e>(0),
                                 KHR_DF_TRANSFER_ITU,
                                 KHR_DF_PRIMARIES_BT709,
                                 tCompCount < 3
                                        ? KHR_DF_MODEL_YUVSDA
                                        : KHR_DF_MODEL_RGBSDA),
                        ImageInputFormatType::npbm);
}


///
/// @internal
/// @~English
/// @brief parse the header of a PGM or PPM file.
///
/// @param [in]  src    pointer to FILE stream to read
/// @param [out] width  reference to a var in which to write the image width.
/// @param [out] height reference to a var in which to write the image height
/// @param [out] maxval reference to a var in which to write the maxval.
///
/// @exception invalid_file if there is no width or height, if maxval is not
///                         an integer or if maxval is out of range.
///
void NpbmInput::parseGPHeader(filetype ftype)
{
    nextLine();
    skipComments();

    uint32_t numvals, width, height, maxVal;

    numvals = sscanf(currentLine.c_str(), "%u %u",
                     &width, &height);
    if (numvals != 2) {
        throw invalid_file("width or height is missing.");
    }
    if (width <= 0 || height <= 0) {
        throw invalid_file("width or height is negative.");
    }

    nextLine();
    skipComments();

    numvals = sscanf(currentLine.c_str(), "%d", &maxVal);
    if (numvals == 0) {
        throw invalid_file("maxval must be an integer.");
    }
    if (maxVal <= 0 || maxVal >= (1<<16)) {
        throw invalid_file("Max color component value must be > 0 && < 65536.");
    }

    images.emplace_back(ImageSpec(width, height, 1,
                                     ftype == filetype::PPM ? 3 : 1, 8,
                                     0, maxVal,
                                     static_cast<khr_df_sample_datatype_qualifiers_e>(0),
                                     KHR_DF_TRANSFER_ITU,
                                     KHR_DF_PRIMARIES_BT709,
                                     ftype == filetype::PPM
                                          ? KHR_DF_MODEL_RGBSDA
                                          : KHR_DF_MODEL_YUVSDA),
                        ImageInputFormatType::npbm);
}


/// @brief Read an entire image into contiguous memory performing conversions
/// to @a requestFormat.
///
/// @sa ImageInput::readScanline() for supported conversions.
void
NpbmInput::readImage(void* pBuffer, size_t bufferByteCount,
                     uint32_t subimage, uint32_t miplevel,
                     const FormatDescriptor& format)
{
    const FormatDescriptor* targetFormat;
    if (isp == nullptr)
        throw std::runtime_error("No open input stream");

    if (bufferByteCount < spec().imageByteCount())
        throw buffer_too_small();

    if (format.isUnknown())
        targetFormat = &spec().format();
    else
        targetFormat = &format;

    if (*targetFormat != spec().format()) {
        // Use default function which reads a scanline at a time to avoid
        // having to buffer entire image for conversion.
        ImageInput::readImage(pBuffer, bufferByteCount,
                              subimage, miplevel,
                              format);
        return;
    }

    try {
        seekSubimage(subimage, miplevel);

        isp->read((char*)pBuffer, spec().imageByteCount());
        curImageScanline = spec().height();
        swap(pBuffer, spec().imageChannelCount());
    } catch (const std::istream::failure&) {
        throwOnReadFailure();
    }
}

