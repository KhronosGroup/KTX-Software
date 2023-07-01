// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"
#include "version.h"
#include "ktx.h"
#include "sbufstream.h"
#include <filesystem>
#include <iostream>

#include <fmt/ostream.h>
#include <fmt/printf.h>


// -------------------------------------------------------------------------------------------------

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

namespace ktx {

void Command::parseCommandLine(const std::string& name, const std::string& desc, int argc, _TCHAR* argv[]) {
    commandName = name;
    commandDescription = desc;

    cxxopts::Options commandOpts(name, "");
    commandOpts.custom_help("[OPTION...]");
    commandOpts.set_width(CONSOLE_USAGE_WIDTH);
    initOptions(commandOpts); // virtual customization point

    cxxopts::ParseResult args;
    try {
        args = commandOpts.parse(argc, argv);
        processOptions(commandOpts, args); // virtual customization point

    } catch (const cxxopts::exceptions::parsing& ex) {
        fatal_usage("{}.", ex.what());
    }
}

std::string version(bool testrun) {
    return testrun ? STR(KTX_DEFAULT_VERSION) : STR(KTX_VERSION);
}

// -------------------------------------------------------------------------------------------------

InputStream::InputStream(const std::string& filepath, Reporter& report) :
        filepath(filepath) {
    if (filepath == "-") {
        #if defined(_WIN32)
        // Set "stdin" to binary mode
        const auto setmodeResult = _setmode(_fileno(stdin), _O_BINARY);
        if (setmodeResult == -1)
            report.fatal(rc::IO_FAILURE, "Failed to set stdin mode to binary: {}.", setmodeResult);
        // #else
        // std::freopen(nullptr, "rb", stdin);
        #endif

        // Read everything from stdin into memory to enable random access
        stdinBuffer << std::cin.rdbuf();
        activeStream = &stdinBuffer;
    } else {
        file.open(filepath, std::ios::binary | std::ios::in);
        if (!file)
            report.fatal(rc::IO_FAILURE, "Could not open input file \"{}\": {}.", filepath, errnoMessage());
        activeStream = &file;
    }
}

// -------------------------------------------------------------------------------------------------

OutputStream::OutputStream(const std::string& filepath, Reporter& report) :
        filepath(filepath) {

    if (filepath == "-") {
        #if defined(_WIN32)
        // Set "stdout" to binary mode
        const auto setmodeResult = _setmode(_fileno(stdout), _O_BINARY);
        if (setmodeResult == -1)
            report.fatal(rc::IO_FAILURE, "Failed to set stdout mode to binary: {}.", setmodeResult);
        // #else
        // std::freopen(nullptr, "wb", stdout);
        #endif
        file = stdout;
    } else {
        file = std::fopen(filepath.c_str(), "wb");
        if (!file)
            report.fatal(rc::IO_FAILURE, "Could not open output file \"{}\": {}.", filepath, errnoMessage());
    }

    // TODO: Investigate and resolve the portability issue with the C++ streams. The issue will most likely
    //       be in StreambufStream's position reporting and seeking. Currently a fallback is implemented in C above.
    // if (filepath == "-") {
    //     #if defined(_WIN32)
    //     // Set "stdout" to binary mode
    //     const auto setmodeResult = _setmode(_fileno(stdout), _O_BINARY);
    //     if (setmodeResult == -1)
    //         report.fatal(rc::IO_FAILURE, "Failed to set stdout mode to binary: {}", setmodeResult);
    //     // #else
    //     // std::freopen(nullptr, "wb", stdout);
    //     #endif
    //     activeStream = &std::cout;
    // } else {
    //     file.open(filepath, std::ios::binary | std::ios::out);
    //     if (!file)
    //         report.fatal(rc::IO_FAILURE, "Could not open output file \"{}\": {}", filepath, errnoMessage());
    //     activeStream = &file;
    // }
}

OutputStream::~OutputStream() {
    if (file != stdout)
        std::fclose(file);
}

void OutputStream::write(const char* data, std::size_t size, Reporter& report) {
    const auto written = std::fwrite(data, 1, size, file);
    if (written != size)
        report.fatal(rc::IO_FAILURE, "Failed to write output file \"{}\": {}.", fmtOutFile(filepath), errnoMessage());
}

void OutputStream::writeKTX2(ktxTexture* texture, Reporter& report) {
    const auto ret = ktxTexture_WriteToStdioStream(texture, file);
    if (KTX_SUCCESS != ret) {
        if (file != stdout)
            std::filesystem::remove(filepath);
        report.fatal(rc::IO_FAILURE, "Failed to write KTX file \"{}\": KTX error: {}.", filepath, ktxErrorString(ret));
    }

    // TODO: Investigate and resolve the portability issue with the C++ streams. The issue will most likely
    //       be in StreambufStream's position reporting and seeking. Currently a fallback is implemented in C above.
    // StreambufStream<std::streambuf*> stream(activeStream->rdbuf(), std::ios::in | std::ios::binary);
    // const auto ret = ktxTexture_WriteToStream(texture, stream.stream());
    //
    // if (KTX_SUCCESS != ret) {
    //     if (activeStream != &std::cout)
    //         std::filesystem::remove(filepath);
    //     report.fatal(rc::IO_FAILURE, "Failed to write KTX file \"{}\": {}.", fmtOutFile(filepath), ktxErrorString(ret));
    // }
}

// -------------------------------------------------------------------------------------------------

} // namespace ktx
