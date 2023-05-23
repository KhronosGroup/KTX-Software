// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "utility.h"

// -------------------------------------------------------------------------------------------------

namespace ktx {

/**
//! [command options_compress]
<dl>
    <dt>--zstd &lt;level&gt;</dt>
    <dd>
        Supercompress the data with Zstandard.
        Cannot be used with ETC1S / BasisLZ format.
        Level range is [1,22].
        Lower levels give faster but worse compression.
        Values above 20 should be used with caution as they require more memory.
    </dd>
    <dt>--zlib &lt;level&gt;</dt>
    <dd>
        Supercompress the data with ZLIB.
        Cannot be used with ETC1S / BasisLZ format.
        Level range is [1,9].
        Lower levels give faster but worse compression.
    </dd>
</dl>
//! [command options_compress]
*/
struct OptionsCompress {
    std::optional<uint32_t> zstd;
    std::optional<uint32_t> zlib;

    void init(cxxopts::Options& opts) {
        opts.add_options()
            ("zstd", "Supercompress the data with Zstandard."
                     " Cannot be used with ETC1S / BasisLZ format."
                     " Level range is [1,22]."
                     " Lower levels give faster but worse compression."
                     " Values above 20 should be used with caution as they require more memory.",
                cxxopts::value<uint32_t>(), "<level>")
            ("zlib", "Supercompress the data with ZLIB."
                     " Cannot be used with ETC1S / BasisLZ format."
                     " Level range is [1,9]."
                     " Lower levels give faster but worse compression.",
                cxxopts::value<uint32_t>(), "<level>");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (args["zstd"].count()) {
            zstd = args["zstd"].as<uint32_t>();
            if (zstd < 1u || zstd > 22u)
                report.fatal_usage("Invalid zstd level: \"{}\". Value must be between 1 and 22 inclusive.", zstd.value());
        }
        if (args["zlib"].count()) {
            zlib = args["zlib"].as<uint32_t>();
            if (zlib < 1u || zlib > 9u)
                report.fatal_usage("Invalid zlib level: \"{}\". Value must be between 1 and 9 inclusive.", zlib.value());
        }
        if (zstd.has_value() && zlib.has_value())
            report.fatal_usage("Conflicting options: zstd and zlib cannot be used at the same time.");
    }
};

} // namespace ktx
