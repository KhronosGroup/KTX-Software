// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file imageinput.cc
//!
//! @brief ImageInput:open function
//!

#include "imageio.h"
#include "imageio_utility.h"
#include "platform_utils.h"

#include <iomanip>
#include <map>
#include <stdarg.h>
#include <stdexcept>
#include <filesystem>


// Search for and instantiate a plugin that can read the format
// and open the file.
std::unique_ptr<ImageInput>
ImageInput::open(const std::string& filename,
                 const ImageSpec* /*config*/,
                 WarningCallbackFunction wcb)
                 //Filesystem::IOProxy* ioproxy, string_view plugin_searchpath)
{
    // Populate inputFormats.
    if (Imageio::inputFormats.empty()) {
        Imageio::catalogBuiltinPlugins();
        assert(!Imageio::inputFormats.empty() && "No image input plugins compiled in.");
    }

    std::ifstream ifs;
    std::unique_ptr<std::stringstream> buffer;
    ImageInput::Creator createFunction = nullptr;
    const std::string* fn;
    const std::string sn("stdin");
    bool doBuffer = true;

    if (filename.compare("-")) {
        // Regular file.
        // Check file exists, before looking for a suitable plugin.
        // MS's STL has `open` overloads that accept wchar_t to handle
        // Window's Unicode file names.
        ifs.open(std::filesystem::path(DecodeUTF8Path(filename)), std::ios::binary | std::ios::in);
        if (ifs.fail()) {
            throw std::runtime_error(
                fmt::format("Open of \"{}\" failed. {}",
                            filename, strerror(errno))
            );
        }

        // Extract the file extension from the filename (without the leading dot)
        Imageio::string format = filename.substr(filename.find_last_of('.')+1);
        format.tolower();

        // Look for a plugin for that extension.
        if (!format.empty()) {
            // First try the plug-in that matches the extension
            Imageio::InputPluginMap::const_iterator found
                       = Imageio::inputFormats.find(format);
            if (found != Imageio::inputFormats.end()) {
                createFunction = found->second;
            }
        }
        fn = &filename;
    } else {
        // cin (stdin)
#if defined(_WIN32)
        // Set "stdin" to have binary mode. There is no way to this via cin.
        (void)_setmode( _fileno( stdin ), _O_BINARY );
        // Windows shells set the FILE_SYNCHRONOUS_IO_NONALERT option when
        // creating pipes. Cygwin since 3.4.x does the same thing, a change
        // which affects anything dependent on it, e.g. Git for Windows
        // (since 2.41.0) and MSYS2. When this option is set, cin.seekg(0)
        // erroneously returns success. Always buffer.
        doBuffer = true;
#else
        // Can we seek in this cin?
        std::cin.seekg(0);
        doBuffer = std::cin.fail();
#endif
        if (doBuffer) {
            // Can't seek. Buffer stdin. This is a potentially large buffer.
            // Must avoid copy. If use stack variable for ss, it's streambuf
            // will also be on the stack and lost after this function exits
            // even with std::move.
            buffer =
                std::unique_ptr<std::stringstream>(new std::stringstream);
            *buffer << std::cin.rdbuf();
            buffer->seekg(0, std::ios::beg);
        }
        fn = &sn;
    }

    // Remember which formats we've already tried, so we don't double dip.
    std::vector<ImageInput::Creator> formatsTried;
    std::string specificError;
    std::unique_ptr<ImageInput> in;

    if (createFunction) {
        formatsTried.push_back(createFunction);
        // Try our guess.
        try {
            in = std::unique_ptr<ImageInput>(createFunction());
            in->connectCallback(wcb);
            ImageSpec tmpspec;
            in->open(*fn, ifs, buffer, tmpspec);
            return in;
        } catch (const std::runtime_error& e) {
            // Oops, it failed.  Apparently, this file can't be
            // opened with this II.
            if (in) {
                specificError = e.what();
                in.reset();
            }
        }
    }

    // A plugin designated for the requested extension either couldn't be
    // found or was unable to open the file or there is no requested extension
    // due to stdin. Try every plugin and see if one will open the file.
    for (auto&& plugin : Imageio::inputFormats) {
        // If we already tried this create function, don't do it again
        if (std::find(formatsTried.begin(), formatsTried.end(), plugin.second)
            != formatsTried.end()) {
            continue;
        }
        formatsTried.push_back(plugin.second);  // remember
        createFunction = plugin.second;
        try {
            in = std::unique_ptr<ImageInput>(createFunction());
            in->connectCallback(wcb);
            ImageSpec tmpspec;
            in->open(*fn, ifs, buffer, tmpspec);
            return in;
        } catch (...) {
            if (in.get()) in.reset();
            continue;
        }
    }

    if (!specificError.empty()) {
        // Pass along any specific error message we got from our
        // best guess of the format.
        throw std::runtime_error(specificError);
    } else {
        throw std::runtime_error(
            fmt::format(
                "No image plugin recognized the format of \"{}\". "
                "Is it a file format that we don't know about?\n",
                filename.compare("-") ? filename : "the data on stdin")
            );
    }
}

/// @brief Open a file for image input.
///
/// Default implementation for derived classes.
void ImageInput::open(const std::string& filename, ImageSpec& newspec)
{
    close(); // previously opened file.
    if (filename.compare("-")) {
        // Regular file
        _filename = filename;
        file.open(filename);
        if (file.fail()) {
            throw std::runtime_error(
                fmt::format("Open of \"{}\" failed. {}",
                            filename, strerror(errno))
            );
        }
        isp = &file;
        open(newspec);
    } else {
        // stdin
        _filename = "stdin";
#if defined(_WIN32)
        /* Set "stdin" to have binary mode */
        (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
        // Can we seek in this cin?
        std::cin.seekg(0);
        if (std::cin.fail()) {
            // Can't seek. Buffer stdin. This is a potentially large buffer.
            // The class uses a unique_ptr due to the constraints of supporting
            // the static create function. See comment in that function above.
            buffer =
                std::unique_ptr<std::stringstream>(new std::stringstream);
            *buffer << std::cin.rdbuf();
            isp = buffer.get();
            open(newspec);
        } else {
            open(std::cin, newspec);
        }
    }
}

/// @brief Read an entire image into contiguous memory performing conversions
/// to @a format.
///
/// Default implementation for derived classes. Input channel values are scaled
/// from the input's channelUpper value to the max representable value of the
/// output format (unorm8 or unorm16).
///
/// Supported conversions are
/// - bit scaling
///   - unorm8<->unorm16
void
ImageInput::readScanline(void* pBufferOut, size_t bufferByteCount,
                         uint32_t y, uint32_t z,
                         uint32_t subimage, uint32_t miplevel,
                         const FormatDescriptor& format)
{
    const auto& targetFormat = format.isUnknown() ? spec().format() : format;

    const auto targetBitLength = targetFormat.largestChannelBitLength();
    const auto requestBits = std::max(imageio::bit_ceil(targetBitLength), 8u);

    if (requestBits != 8 && requestBits != 16)
        throw std::runtime_error(fmt::format(
                "Requested decode into {}-bit format is not supported.",
                requestBits)
              );

    const bool targetL = targetFormat.samples[0].qualifierLinear;
    const bool targetE = targetFormat.samples[0].qualifierExponent;
    const bool targetS = targetFormat.samples[0].qualifierSigned;
    const bool targetF = targetFormat.samples[0].qualifierFloat;

    // Only UNORM requests are supported here.
    if (targetE || targetL || targetS || targetF)
        throw std::runtime_error(fmt::format(
                "Requested format conversion to {}-bit{}{}{}{} is not supported.",
                requestBits,
                targetL ? " Linear" : "",
                targetE ? " Exponent" : "",
                targetS ? " Signed" : "",
                targetF ? " Float" : "")
              );

    seekSubimage(subimage, miplevel);

    size_t imageByteCount = (requestBits * spec().imageChannelCount()) / 8;
    if (imageByteCount < bufferByteCount)
        throw buffer_too_small();

    uint8_t* pNativeBuffer;
    if (targetFormat.channelBitLength() != spec().format().channelBitLength()) {
        if (spec().format().channelBitLength() == 16) {
            if (nativeBuffer16.size() < spec().imageChannelCount())
                nativeBuffer16.resize(spec().imageChannelCount());
            pNativeBuffer = reinterpret_cast<uint8_t*>(nativeBuffer16.data());
            bufferByteCount = nativeBuffer16.size() * sizeof(uint16_t);
        } else {
            if (nativeBuffer8.size() < spec().imageChannelCount())
                nativeBuffer8.resize(spec().imageChannelCount());
            pNativeBuffer = nativeBuffer8.data();
            bufferByteCount = nativeBuffer16.size() * sizeof(uint8_t);
        }
    } else {
        pNativeBuffer = static_cast<uint8_t*>(pBufferOut);
    }

    readNativeScanline(pNativeBuffer, bufferByteCount, y, z, subimage, miplevel);

    if (reinterpret_cast<uint16_t*>(pNativeBuffer) == nativeBuffer16.data()) {
         rescale(static_cast<uint8_t*>(pBufferOut),
                 static_cast<uint8_t>(targetFormat.channelUpper()),
                 nativeBuffer16.data(),
                 static_cast<uint16_t>(spec().format().channelUpper()),
                 spec().imageChannelCount());
    } else if (pNativeBuffer == nativeBuffer8.data()) {
         rescale(static_cast<uint16_t*>(pBufferOut),
                 static_cast<uint16_t>(targetFormat.channelUpper()),
                 nativeBuffer8.data(),
                 static_cast<uint8_t>(spec().format().channelUpper()),
                 spec().imageChannelCount());
    } else if (targetFormat.channelUpper() != spec().format().channelUpper()) {
        if (spec().format().channelBitLength() == 16) {
            rescale(static_cast<uint16_t*>(pBufferOut),
                    static_cast<uint16_t>(targetFormat.channelUpper()),
                    static_cast<uint16_t*>(pBufferOut),
                    static_cast<uint16_t>(spec().format().channelUpper()),
                    spec().imageChannelCount());
        } else {
            rescale(static_cast<uint8_t*>(pBufferOut),
                    static_cast<uint8_t>(targetFormat.channelUpper()),
                    static_cast<uint8_t*>(pBufferOut),
                    static_cast<uint8_t>(spec().format().channelUpper()),
                    spec().imageChannelCount());
        }
    }
}


/// @brief Read an entire image into contiguous memory performing conversions
/// to @a format.
///
/// Default implementation for derived classes.
///
/// @sa readScanline() for support conversions.
void
ImageInput::readImage(void* pBuffer, size_t bufferByteCount,
                     uint32_t subimage, uint32_t miplevel,
                     const FormatDescriptor& format)
{
    const auto& targetFormat = format.isUnknown() ? spec().format() : format;
    size_t outScanlineByteCount
           = targetFormat.pixelByteCount() * spec().width();
    if (bufferByteCount < outScanlineByteCount * spec().height())
        throw buffer_too_small();

    uint8_t* pDst = static_cast<uint8_t*>(pBuffer);
    for (uint32_t y = 0; y < spec().height(); y++) {
        readScanline(pDst, bufferByteCount,
                     y, 0, subimage, miplevel, targetFormat);
        pDst += outScanlineByteCount;
        bufferByteCount -= outScanlineByteCount;
    }
}

void ImageInput::throwOnReadFailure()
{
    if (isp->eof()) {
        throw std::runtime_error("Unexpected end-of-file.");
    } else {
        throw std::runtime_error(
            fmt::format("I/O error reading file: {}", strerror(errno))
        );
    }
}

void ImageInput::warning(const std::string& wmsg)
{
    if (sendWarning) {
        sendWarning(wmsg);
    }
}

void ImageInput::fwarning(const std::string& wmsg)
{
    std::string fwmsg = _filename + ": ";
    fwmsg += wmsg;
    warning(fwmsg);
}

