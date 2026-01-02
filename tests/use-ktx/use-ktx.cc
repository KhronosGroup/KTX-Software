// Copyright 2025 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <format>
#include <ktx.h>
#include <KHR/khr_df.h>

int main(int argc, char* argv[])
{
    ktxTexture2* tex;
    ktx_error_code_e result;

    if (argc < 2) {
        std::cerr << std::format("{}: Need a file to open\n", argv[0]);
        return 1;
    }
    result = ktxTexture2_CreateFromNamedFile(argv[1], KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex);
    if (result != KTX_SUCCESS) {
        std::cerr << std::format("Could not open {}: {}\n", argv[1], ktxErrorString(result));
        return 1;
    }
    ktxTexture2_Destroy(tex);
    return 0;
}
