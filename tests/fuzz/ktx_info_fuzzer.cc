// Copyright 2026 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// libFuzzer target for the info printers of libktx.
//
// ktxPrintInfoForMemory prints a KTX or KTX2 file as text and
// ktxPrintKTX2InfoJSONForMemory prints a KTX2 file as JSON, as ktx info
// does. The printers read the header and the blocks it describes
// themselves, without the checks ktxTexture_CreateFromMemory makes, so
// ktx_read_fuzzer does not reach them. See README.md.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <ktx.h>

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/) {
    // The printers write to stdout. Printing every input would slow the
    // fuzzer down and bury its own output, which goes to stderr.
#ifdef _WIN32
    const char* nullDevice = "NUL";
#else
    const char* nullDevice = "/dev/null";
#endif
    if (std::freopen(nullDevice, "w", stdout) == nullptr) {
        // freopen has closed stdout, which the printers would then use.
        std::perror("Could not discard stdout");
        std::exit(EXIT_FAILURE);
    }
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    (void)ktxPrintInfoForMemory(data, size);
    // The minified output differs only in white space, so print it for
    // every other input.
    const bool minified = size % 2 != 0;
    (void)ktxPrintKTX2InfoJSONForMemory(data, size, 0, 4, minified);
    return 0;
}
