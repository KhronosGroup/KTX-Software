// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "ktx.h"
#include "imageio_utility.h"
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <type_traits>


// -------------------------------------------------------------------------------------------------

namespace ktx {

// TODO: Detect endianness
// C++20: std::endian::native == std::endian::little
constexpr bool is_big_endian = false;

template <typename T>
[[nodiscard]] constexpr inline T align(const T value, const T alignment) noexcept {
    assert(alignment != 0);
    return (alignment - 1 + value) / alignment * alignment;
}

template <typename T>
[[nodiscard]] constexpr inline T* align(T* ptr, std::uintptr_t alignment) noexcept {
    return reinterpret_cast<T*>(align(reinterpret_cast<std::uintptr_t>(ptr), alignment));
}

template <typename T>
[[nodiscard]] constexpr inline T ceil_div(const T x, const T y) noexcept {
    assert(y != 0);
	return (x + y - 1) / y;
}

/// log2 floor
[[nodiscard]] constexpr inline uint32_t log2(uint32_t v) noexcept {
    uint32_t e = 0;

    // http://aggregate.org/MAGIC/
    v |= (v >> 1);
    v |= (v >> 2);
    v |= (v >> 4);
    v |= (v >> 8);
    v |= (v >> 16);
    v = v & ~(v >> 1);

    e = (v & 0xAAAAAAAA) ? 1 : 0;
    e |= (v & 0xCCCCCCCC) ? 2 : 0;
    e |= (v & 0xF0F0F0F0) ? 4 : 0;
    e |= (v & 0xFF00FF00) ? 8 : 0;
    e |= (v & 0xFFFF0000) ? 16 : 0;

    return e;
}

// C++20 - std::popcount
template <typename T>
[[nodiscard]] constexpr inline int popcount(T value) noexcept {
    int count = 0;
    for (; value != 0; value >>= 1)
        if (value & 1)
            count++;
    return count;
}

// C++20 - std::to_underlying
template <typename E>
[[nodiscard]] constexpr inline auto to_underlying(E e) noexcept {
    static_assert(std::is_enum_v<E>, "E has to be an enum type");
    return static_cast<std::underlying_type_t<E>>(e);
}

// C++20 - std::byteswap
template <typename T>
[[nodiscard]] constexpr inline T byteswap(T value) noexcept {
    static_assert(std::has_unique_object_representations_v<T>, "T may have padding bits");
    auto representation = imageio::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    for (uint32_t i = 0; i < representation.size() / 2u; ++i)
        std::swap(representation[i], representation[representation.size() - 1 - i]);
    return imageio::bit_cast<T>(representation);
}

// C++20 - string.starts_with(prefix)
[[nodiscard]] constexpr inline bool starts_with(std::string_view string, std::string_view prefix) noexcept {
    return prefix == string.substr(0, prefix.size());
}

// C++20 - string.contains(char)
[[nodiscard]] constexpr inline bool contains(std::string_view string, char c) noexcept {
    return string.find(c) != std::string_view::npos;
}

/// C++20 - std::identity
/// identity is a function object type whose operator() returns its argument unchanged.
struct identity {
    using is_transparent = void;

    template <typename T>
    [[nodiscard]] constexpr inline T&& operator()(T&& arg) const noexcept {
        return std::forward<T>(arg);
    }
};

// -------------------------------------------------------------------------------------------------

template <typename Range, typename Comp = std::less<>, typename Proj = identity>
[[nodiscard]] constexpr inline bool is_sorted(const Range& range, Comp&& comp = {}, Proj&& proj = {}) {
    return std::is_sorted(std::begin(range), std::end(range), [&](const auto& lhs, const auto& rhs) {
        return comp(std::invoke(proj, lhs), std::invoke(proj, rhs));
    });
}

template <typename Range, typename Comp = std::less<>, typename Proj = identity>
constexpr inline void sort(Range& range, Comp&& comp = {}, Proj&& proj = {}) {
    return std::sort(std::begin(range), std::end(range), [&](const auto& lhs, const auto& rhs) {
        return comp(std::invoke(proj, lhs), std::invoke(proj, rhs));
    });
}

inline void to_lower_inplace(std::string& string) {
    for (auto& c : string)
        c = static_cast<char>(std::tolower(c));
}

[[nodiscard]] inline std::string to_lower_copy(std::string string) {
    to_lower_inplace(string);
    return string;
}

inline void to_upper_inplace(std::string& string) {
    for (auto& c : string)
        c = static_cast<char>(std::toupper(c));
}

[[nodiscard]] inline std::string to_upper_copy(std::string string) {
    to_upper_inplace(string);
    return string;
}

inline void replace_all_inplace(std::string& string, std::string_view search, std::string_view replace) {
    auto pos = string.find(search);
    while (pos != std::string::npos) {
        string.replace(pos, search.size(), replace);
        pos = string.find(search, pos + replace.size());
    }
}

[[nodiscard]] inline std::string replace_all_copy(std::string string, std::string_view search, std::string_view replace) {
    replace_all_inplace(string, search, replace);
    return string;
}

[[nodiscard]] inline std::string escape_json_copy(std::string string) {
    replace_all_inplace(string, "\\", "\\\\");
    replace_all_inplace(string, "\"", "\\\"");
    replace_all_inplace(string, "\n", "\\n");
    return string;
}

// -------------------------------------------------------------------------------------------------

/// Remaps the value from [from_lo..from_hi] to [to_lo..to_hi] with extrapolation
template <typename T>
[[nodiscard]] constexpr inline T remap(T value, T from_lo, T from_hi, T to_lo, T to_hi) noexcept {
	return to_lo + (value - from_lo) * (to_hi - to_lo) / (from_hi - from_lo);
}

// --- UTF-8 ---------------------------------------------------------------------------------------

/**
 * @internal
 * @brief Given the lead byte of a UTF-8 sequence returns the expected length of the codepoint
 * @param[in] leadByte The lead byte of a UTF-8 sequence
 * @return The expected length of the codepoint */
[[nodiscard]] constexpr inline int sequenceLength(uint8_t leadByte) noexcept {
    if ((leadByte & 0b1000'0000u) == 0b0000'0000u)
        return 1;
    if ((leadByte & 0b1110'0000u) == 0b1100'0000u)
        return 2;
    if ((leadByte & 0b1111'0000u) == 0b1110'0000u)
        return 3;
    if ((leadByte & 0b1111'1000u) == 0b1111'0000u)
        return 4;

    return 0;
}

/**
 * @internal
 * @brief Checks if the codepoint was coded as a longer than required sequence
 * @param[in] codepoint The unicode codepoint
 * @param[in] length The UTF-8 sequence length
 * @return True if the sequence length was inappropriate for the given codepoint */
[[nodiscard]] constexpr inline bool isOverlongSequence(uint32_t codepoint, int length) noexcept {
    if (codepoint < 0x80)
        return length != 1;
    else if (codepoint < 0x800)
        return length != 2;
    else if (codepoint < 0x10000)
        return length != 3;
    else
        return false;
}

/**
 * @internal
 * @brief Checks if the codepoint is valid
 * @param[in] codepoint The unicode codepoint
 * @return True if the codepoint is a valid unicode codepoint */
[[nodiscard]] constexpr inline bool isCodepointValid(uint32_t codepoint) noexcept {
    return codepoint <= 0x0010FFFFu
            && !(0xD800u <= codepoint && codepoint <= 0xDBFFu);
}

/**
 * @internal
 * @brief Safely checks and advances a UTF-8 sequence iterator to the start of the next unicode codepoint
 * @param[in] it iterator to be advanced
 * @param[in] end iterator pointing to the end of the range
 * @return True if the advance operation was successful and the advanced codepoint was a valid UTF-8 sequence */
template <typename Iterator>
[[nodiscard]] constexpr bool advanceUTF8(Iterator& it, Iterator end) noexcept {
    if (it == end)
        return false;

    const auto length = sequenceLength(*it);
    if (length == 0)
        return false;

    if (std::distance(it, end) < length)
        return false;

    for (int i = 1; i < length; ++i) {
        const auto trailByte = *(it + i);
        if ((static_cast<uint8_t>(trailByte) & 0b1100'0000u) != 0b1000'0000u)
            return false;
    }

    uint32_t codepoint = 0;
    switch (length) {
    case 1:
        codepoint |= *it++;
        break;
    case 2:
        codepoint |= (*it++ & 0b0001'1111u) << 6u;
        codepoint |= (*it++ & 0b0011'1111u);
        break;
    case 3:
        codepoint |= (*it++ & 0b0000'1111u) << 12u;
        codepoint |= (*it++ & 0b0011'1111u) << 6u;
        codepoint |= (*it++ & 0b0011'1111u);
        break;
    case 4:
        codepoint |= (*it++ & 0b0000'0111u) << 18u;
        codepoint |= (*it++ & 0b0011'1111u) << 12u;
        codepoint |= (*it++ & 0b0011'1111u) << 6u;
        codepoint |= (*it++ & 0b0011'1111u);
        break;
    }

    if (!isCodepointValid(codepoint))
        return false;

    if (isOverlongSequence(codepoint, length))
        return false;

    return true;
}

/**
 * @internal
 * @brief Validates a UTF-8 sequence
 * @param[in] text The string to be validated
 * @return nullopt if the sequence is valid otherwise the first index where an invalid UTF-8 character was found */
[[nodiscard]] constexpr inline std::optional<std::size_t> validateUTF8(std::string_view text) noexcept {
    auto it = text.begin();
    const auto end = text.end();

    while (it != end) {
        if (!advanceUTF8(it, end))
            return std::distance(text.begin(), it);
    }

    return std::nullopt;
}

// -------------------------------------------------------------------------------------------------

[[nodiscard]] inline std::string fmtInFile(std::string_view filepath) {
    return filepath == "-" ? std::string("stdin") : std::string(filepath);
}

[[nodiscard]] inline std::string fmtOutFile(std::string_view filepath) {
    return filepath == "-" ? std::string("stdout") : std::string(filepath);
}

// -------------------------------------------------------------------------------------------------

struct PrintIndent {
    std::ostream& os;
    int indentBase = 0;
    int indentWidth = 4;

public:
    template <typename Fmt, typename... Args>
    inline void operator()(int depth, Fmt&& fmt, Args&&... args) {
        fmt::print(os, "{:{}}", "", indentWidth * (indentBase + depth));
        fmt::print(os, std::forward<Fmt>(fmt), std::forward<Args>(args)...);
    }
};

[[nodiscard]] inline std::string errnoMessage() {
    return std::make_error_code(static_cast<std::errc>(errno)).message();
}

// -------------------------------------------------------------------------------------------------

/// RAII Handler for ktxTexture
class KTXTexture2 final {
private:
    ktxTexture2* handle_ = nullptr;

public:
    explicit KTXTexture2(std::nullptr_t) : handle_{nullptr} { }
    explicit KTXTexture2(ktxTexture2* handle) : handle_{handle} { }

    KTXTexture2(const KTXTexture2&) = delete;
    KTXTexture2& operator=(const KTXTexture2&) = delete;

    KTXTexture2(KTXTexture2&& other) noexcept :
        handle_{other.handle_} {
        other.handle_ = nullptr;
    }

    KTXTexture2& operator=(KTXTexture2&& other) & {
        handle_ = other.handle_;
        other.handle_ = nullptr;
        return *this;
    }

    ~KTXTexture2() {
        if (handle_ != nullptr) {
            ktxTexture_Destroy(ktxTexture(handle_));
            handle_ = nullptr;
        }
    }

    inline ktxTexture2* handle() const {
        return handle_;
    }

    inline ktxTexture2** pHandle() {
        return &handle_;
    }

    /*implicit*/ inline operator ktxTexture*() {
        return ktxTexture(handle_);
    }

    /*implicit*/ inline operator ktxTexture2*() {
        return handle_;
    }

    inline ktxTexture2* operator->() const {
        return handle_;
    }
};


template <typename T>
struct ClampedOption {
    ClampedOption(T& option, T min_v, T max_v) : option(option), min(min_v), max(max_v) {}

    void clear() {
        option = 0;
    }

    operator T() const {
        return option;
    }

    T operator=(T v) {
        option = std::clamp<T>(v, min, max);
        return option;
    }

    T& option;
    T min;
    T max;
};

} // namespace ktx
