// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <fmt/format.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>


// -------------------------------------------------------------------------------------------------

namespace ktx {

struct All_t {};
static constexpr All_t all{};

using RangeIndex = uint32_t;
static constexpr RangeIndex RangeEnd = std::numeric_limits<RangeIndex>::max();

class SelectorRange {
public:
    // Half open range [begin, end)
    struct HalfRange {
        RangeIndex begin = 0;
        RangeIndex end = RangeEnd;

        [[nodiscard]] friend constexpr bool operator==(const HalfRange& lhs, const HalfRange& rhs) {
            return lhs.begin == rhs.begin && lhs.end == rhs.end;
        }
    };

private:
    std::vector<HalfRange> ranges;

public:
    SelectorRange(RangeIndex begin, RangeIndex end) {
        ranges.emplace_back(HalfRange{begin, end});
    }
    SelectorRange() = default;
    explicit SelectorRange(All_t) : SelectorRange(0, RangeEnd) {};
    explicit SelectorRange(RangeIndex index) : SelectorRange(index, index + 1u) {};

    [[nodiscard]] friend bool operator==(const SelectorRange& lhs, const SelectorRange& rhs) {
        return lhs.ranges == rhs.ranges;
    }

    [[nodiscard]] bool is_empty() const {
        if (ranges.empty())
            return true;
        for (const auto& range : ranges)
            if (range.begin != range.end)
                return false;
        return true;
    }

    [[nodiscard]] bool is_single() const {
        if (ranges.empty())
            return false;
        const auto index = ranges[0].begin;
        for (const auto& range : ranges)
            if (range.begin != index || range.end != index + 1)
                return false;
        return true;
    }

    [[nodiscard]] bool is_multi() const {
        return !is_single();
    }

    void clear() {
        ranges.clear();
    }

    [[nodiscard]] bool is_undefined() const {
        return ranges.empty();
    }

    void add(HalfRange range) {
        ranges.emplace_back(range);
    }

    [[nodiscard]] bool contains(RangeIndex index) const {
        for (const auto& range : ranges)
            if (range.begin <= index && index < range.end)
                return true;
        return false;
    }

    [[nodiscard]] bool validate(RangeIndex last) const {
        for (const auto& range : ranges) {
            if (range.begin > last)
                return false;
            if (range.end > last && range.end != RangeEnd)
                return false;
        }
        return true;
    }

    [[nodiscard]] RangeIndex last() const {
        RangeIndex last = 0;
        for (const auto& range : ranges)
            if (range.begin != range.end && range.end > last + 1)
                last = range.end - 1;
        return last;
    }

    // Only used for fmt print
    [[nodiscard]] const std::vector<HalfRange>& _ranges() const {
        return ranges;
    }

    SelectorRange& operator=(All_t) & {
        ranges.clear();
        ranges.emplace_back(HalfRange{0, RangeEnd});
        return *this;
    }

    SelectorRange& operator=(RangeIndex index) & {
        ranges.clear();
        ranges.emplace_back(HalfRange{index, index + 1});
        return *this;
    }

    friend bool operator==(const SelectorRange& var, All_t) {
        return var == SelectorRange{all};
    }
    friend bool operator==(All_t, const SelectorRange& var) {
        return var == all;
    }
    friend bool operator!=(const SelectorRange& var, All_t) {
        return !(var == all);
    }
    friend bool operator!=(All_t, const SelectorRange& var) {
        return !(var == all);
    }
    friend bool operator==(const SelectorRange& var, RangeIndex index) {
        return var == SelectorRange{index};
    }
    friend bool operator==(RangeIndex index, const SelectorRange& var) {
        return var == index;
    }
};

} // namespace ktx

namespace fmt {

template<> struct formatter<ktx::SelectorRange> : fmt::formatter<uint32_t> {
    template <typename FormatContext>
    auto format(const ktx::SelectorRange& var, FormatContext& ctx) const -> decltype(ctx.out()) {
        if (var == ktx::all)
            return formatter<std::string_view>{}.format("all", ctx);
        else if (var.is_empty())
            return formatter<std::string_view>{}.format("none", ctx);
        else {
            bool first = true;
            auto out = ctx.out();
            for (const auto& range : var._ranges()) {
                if (!first) {
                    out = fmt::format_to(out, ",");
                    ctx.advance_to(out);
                }
                first = false;

                if (range.begin + 1 == range.end) {
                    out = fmt::format_to(out, "{}", range.begin);
                    ctx.advance_to(out);
                } else if (range.end == ktx::RangeEnd) {
                    out = fmt::format_to(out, "{}..last", range.begin);
                    ctx.advance_to(out);
                } else {
                    out = fmt::format_to(out, "{}..{}", range.begin, range.end - 1);
                    ctx.advance_to(out);
                }
            }

            return out;
        }
    }
};

} // namespace fmt

namespace ktx {

/// https://registry.khronos.org/KTX/specs/2.0/ktx-frag.html
///
/// KTX fragments support addressing the KTX file’s payload along 5 dimensions
///     mip - This dimension denotes a range of mip levels in the KTX file.
///     stratal - This dimension denotes a range of array layers when the KTX file contains an array texture.
///     temporal - This dimension denotes a specific time range in a KTX file containing KTXanimData metadata. Since a frame is an array layer, this is an alternate way of selecting in the stratal dimension.
///     facial - This dimension denotes a range of faces when the KTX file contains a cube map.
///     spatial - xyzwhd This dimension denotes a range of pixels in the KTX file such as "a volume with size (100,100,1) with its origin at (10,10,0).
struct FragmentURI {
    SelectorRange mip;
    SelectorRange stratal;
    // SelectorTemporal temporal; // Temporal selector is outside the current scope
    SelectorRange facial;
    // SelectorSpatial spatial; // Spatial selector is outside the current scope

    bool validate(uint32_t numLevels, uint32_t numLayers, uint32_t numFaces) {
        return mip.validate(numLevels) &&
                stratal.validate(numLayers) &&
                facial.validate(numFaces);
    }
};

[[nodiscard]] inline SelectorRange::HalfRange parseHalfRange(std::string_view key, std::string_view str) {
    const auto indexComma = str.find(',');
    const auto strBegin = indexComma == std::string_view::npos ? str : str.substr(0, indexComma);
    const auto strEnd = indexComma == std::string_view::npos ? "" : str.substr(indexComma + 1);

    try {
        const auto begin = strBegin.empty() ? 0u : static_cast<uint32_t>(std::stoi(std::string(strBegin)));
        const auto end = strEnd.empty() ? RangeEnd : static_cast<uint32_t>(std::stoi(std::string(strEnd))) + 1u;
        return {begin, end};
    } catch (const std::exception& ex) {
        throw std::invalid_argument(fmt::format("Invalid key-value \"{}={}\": {}", key, str, ex.what()));
    }
}

[[nodiscard]] inline FragmentURI parseFragmentURI(std::string_view str) {
    // Name and value components are separated by an equal sign (=),
    // while multiple name-value pairs are separated by an ampersand (&).

    FragmentURI result;
    result.mip.clear();
    result.stratal.clear();
    result.facial.clear();

    while (true) {
        const auto indexAmp = str.find('&');
        const auto strKeyValue = indexAmp == std::string_view::npos ? str : str.substr(0, indexAmp);

        const auto indexEqual = strKeyValue.find('=');
        const auto strKey = indexEqual == std::string_view::npos ? strKeyValue : strKeyValue.substr(0, indexEqual);
        const auto strValue = indexEqual == std::string_view::npos ? "" : strKeyValue.substr(indexEqual + 1);

        if (strKey == "m" || strKey == "%6D")
            result.mip.add(parseHalfRange(strKey, strValue));
        else if (strKey == "a" || strKey == "%61")
            result.stratal.add(parseHalfRange(strKey, strValue));
        else if (strKey == "t" || strKey == "%74")
            throw std::invalid_argument(fmt::format("Temporal selector (t) is not yet supported."));
        else if (strKey == "f")
            result.facial.add(parseHalfRange(strKey, strValue));
        else if (strKey == "xyzwhd")
            throw std::invalid_argument(fmt::format("Spatial selector (xyzwhd) is not yet supported."));
        else if (!strKey.empty())
            throw std::invalid_argument(fmt::format("Unknown key \"{}\"", strKey));

        if (indexAmp != std::string_view::npos)
            str.remove_prefix(indexAmp + 1);
        else
            break;
    }

    return result;
}

} // namespace ktx
