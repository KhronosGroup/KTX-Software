// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "utility.h"
#include "validate.h"
#include <iostream>
#include <sstream>
#include <utility>

#include <cxxopts.hpp>
#include <fmt/printf.h>


// -------------------------------------------------------------------------------------------------

namespace ktx {

/** @page ktx_validate ktx validate
@~English

Validate a KTX2 file.

@section ktx_validate_synopsis SYNOPSIS
    ktx validate [option...] @e input-file

@section ktx_validate_description DESCRIPTION
    @b ktx @b validate validates the Khronos texture format version 2 (KTX2) file specified
    as the @e input-file argument. It prints any found errors and warnings to stdout.
    If the @e input-file is '-' the file will be read from the stdin.

    The validation rules and checks are based on the official specification:
    KTX File Format Specification - https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html

    The JSON output formats conform to the https://schema.khronos.org/ktx/validate_v0.json
    schema.

    @note @b ktx @b validate prints using UTF-8 encoding. If your console is not
    set for UTF-8 you will see incorrect characters in output of the file
    identifier on each side of the "KTX nn".

    The following options are available:
    @snippet{doc} ktx/command.h command options_format
    <dl>
        <dt>-g, \--gltf-basisu</dt>
        <dd>Check compatibility with KHR_texture_basisu glTF extension.</dd>
        <dt>-e, \--warnings-as-errors</dt>
        <dd>Treat warnings as errors.</dd>
    </dl>
    @snippet{doc} ktx/command.h command options_generic

@section ktx_validate_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_validate_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_validate_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandValidate : public Command {
    struct OptionsValidate {
        bool warningsAsErrors = false;
        bool GLTFBasisU = false;

        void init(cxxopts::Options& opts) {
            opts.add_options()
                ("e,warnings-as-errors", "Treat warnings as errors.")
                ("g,gltf-basisu", "Check compatibility with KHR_texture_basisu glTF extension.");
        }

        void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter&) {
            warningsAsErrors = args["warnings-as-errors"].as<bool>();
            GLTFBasisU = args["gltf-basisu"].as<bool>();
        }
    };

    Combine<OptionsValidate, OptionsFormat, OptionsSingleIn, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeValidate();
};

// -------------------------------------------------------------------------------------------------

int CommandValidate::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx validate",
                "Validates the Khronos texture format version 2 (KTX2) file specified\n"
                "    as the input-file argument. It prints any found errors and warnings to stdout.",
                argc, argv);
        executeValidate();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandValidate::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandValidate::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
}

void CommandValidate::executeValidate() {
    InputStream inputStream(options.inputFilepath, *this);

    switch (options.format) {
    case OutputFormat::text: {
        std::ostringstream messagesOS;
        const auto validationResult = validateIOStream(
                inputStream,
                fmtInFile(options.inputFilepath),
                options.warningsAsErrors,
                options.GLTFBasisU,
                [&](const ValidationReport& issue) {
            fmt::print(messagesOS, "{}-{:04}: {}\n", toString(issue.type), issue.id, issue.message);
            fmt::print(messagesOS, "    {}\n", issue.details);
        });

        const auto validationMessages = std::move(messagesOS).str();
        if (!validationMessages.empty()) {
            fmt::print("Validation {}\n", validationResult == 0 ? "successful" : "failed");
            fmt::print("\n");
            fmt::print("{}", validationMessages);
        }

        if (validationResult != 0)
            throw FatalError(rc::INVALID_FILE);
        break;
    }
    case OutputFormat::json: [[fallthrough]];
    case OutputFormat::json_mini: {
        const auto base_indent = options.format == OutputFormat::json ? +0 : 0;
        const auto indent_width = options.format == OutputFormat::json ? 4 : 0;
        const auto space = options.format == OutputFormat::json ? " " : "";
        const auto nl = options.format == OutputFormat::json ? "\n" : "";

        std::ostringstream messagesOS;
        PrintIndent pi{messagesOS, base_indent, indent_width};

        bool first = true;
        const auto validationResult = validateIOStream(
                inputStream,
                fmtInFile(options.inputFilepath),
                options.warningsAsErrors,
                options.GLTFBasisU,
                [&](const ValidationReport& issue) {
            if (!std::exchange(first, false)) {
                pi(2, "}},{}", nl);
            }
            pi(2, "{{{}", nl);
            pi(3, "\"id\":{}{},{}", space, issue.id, nl);
            pi(3, "\"type\":{}\"{}\",{}", space, toString(issue.type), nl);
            pi(3, "\"message\":{}\"{}\",{}", space, escape_json_copy(issue.message), nl);
            pi(3, "\"details\":{}\"{}\"{}", space, escape_json_copy(issue.details), nl);
        });

        PrintIndent out{std::cout, base_indent, indent_width};
        out(0, "{{{}", nl);
        out(1, "\"$schema\":{}\"https://schema.khronos.org/ktx/validate_v0.json\",{}", space, nl);
        out(1, "\"valid\":{}{},{}", space, validationResult == 0, nl);
        if (!first) {
            out(1, "\"messages\":{}[{}", space, nl);
            fmt::print("{}", std::move(messagesOS).str());
            out(2, "}}{}", nl);
            out(1, "]{}", nl);
        } else {
            out(1, "\"messages\":{}[]{}", space, nl);
        }
        out(0, "}}{}", nl);

        if (validationResult != 0)
            throw FatalError(rc::INVALID_FILE);
        break;
    }
    }
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxValidate, ktx::CommandValidate)
