// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "utility.h"

#include <thread>
#include <unordered_map>

// -------------------------------------------------------------------------------------------------

namespace ktx {

/**
//! [command options_codec_common]
    <dt>
        common:
    </dt>
    <dd>
        Common options.
    </dd>

    <dl>
        <dt>\--normal-mode</dt>
        <dd>Only valid for linear textures with two or more components.
            If the input texture has three or four linear components it is
            assumed to be a three component linear normal map storing unit
            length normals as (R=X, G=Y, B=Z). A fourth component will be
            ignored. The map will be converted to a two component X+Y
            normal map stored as (RGB=X, A=Y) prior to encoding. If unsure
            that your normals are unit length, use @b --normalize. If the
            input has 2 linear components it is assumed to be an X+Y map
            of unit normals.

            The Z component can be recovered programmatically in shader
            code by using the equations:
            <pre>
    nml.xy = texture(...).ga;              // Load in [0,1]
    nml.xy = nml.xy * 2.0 - 1.0;           // Unpack to [-1,1]
    nml.z = sqrt(1 - dot(nml.xy, nml.xy)); // Compute Z
            </pre>
            For ETC1S / BasisLZ encoding, @b '--encode basis-lz', RDO is disabled
            (no selector RDO, no endpoint RDO) to provide better quality.</dd>
        <dt>\--threads &lt;count&gt;</dt>
        <dd>Explicitly set the number of threads to use during
            compression. By default, ETC1S / BasisLZ will use the number of
            threads reported by thread::hardware_concurrency or 1 if value
            returned is 0.</dd>
    </dl>
//! [command options_codec_common]
*/
struct OptionsCodecCommon {
    inline static const char* kNormalMode = "normal-mode";
    inline static const char* kThreads = "threads";

    std::string commonOptions{};
    bool normalMap{false};
    ktx_uint32_t threadCount{1};

    OptionsCodecCommon() {
        threadCount = std::clamp<ktx_uint32_t>(threadCount, std::thread::hardware_concurrency(), 10000);
    }

    void init(cxxopts::Options& opts) {
        opts.add_options("Encode common")
            (kNormalMode, "Optimizes for encoding textures with normal data. If the input texture has "
                "three or four linear components it is assumed to be a three component linear normal "
                "map storing unit length normals as (R=X, G=Y, B=Z). A fourth component will be ignored. "
                "The map will be converted to a two component X+Y normal map stored as (RGB=X, A=Y) "
                "prior to encoding. If unsure that your normals are unit length, use --normalize. "
                "If the input has 2 linear components it is assumed to be an X+Y map of unit normals.\n"
                "The Z component can be recovered programmatically in shader code by using the equations:\n"
                "    nml.xy = texture(...).ga;              // Load in [0,1]\n"
                "    nml.xy = nml.xy * 2.0 - 1.0;           // Unpack to [-1,1]\n"
                "    nml.z = sqrt(1 - dot(nml.xy, nml.xy)); // Compute Z\n"
                "ETC1S / BasisLZ encoding, RDO is disabled (no selector RDO, no endpoint RDO) to provide better quality.")
            (kThreads, "Sets the number of threads to use during encoding. By default, encoding "
                "will use the number of threads reported by thread::hardware_concurrency or 1 if "
                "value returned is 0.", cxxopts::value<uint32_t>(), "<count>");
    }

    void captureCodecOption(const char* name) {
        commonOptions += fmt::format(" --{}", name);
    }

    template <typename T>
    T captureCodecOption(cxxopts::ParseResult& args, const char* name) {
        const T value = args[name].as<T>();
        commonOptions += fmt::format(" --{} {}", name, value);
        return value;
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter&) {
        if (args[kNormalMode].count()) {
            captureCodecOption(kNormalMode);
            normalMap = true;
        }

        if (args[kThreads].count()) {
            threadCount = captureCodecOption<uint32_t>(args, kThreads);
        }
    }
};

template <typename Options, typename Codec>
void fillCodecOptions(Options &options) {
    options.Codec::threadCount = options.OptionsCodecCommon::threadCount;
    options.Codec::normalMap = options.OptionsCodecCommon::normalMap;
}

} // namespace ktx
