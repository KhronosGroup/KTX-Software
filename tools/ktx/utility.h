// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "ktx.h"
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <type_traits>


// -------------------------------------------------------------------------------------------------

namespace ktx {

// TODO: Tools P5: Detect endianness
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

// C++20 - std::bit_ceil
template <typename T>
[[nodiscard]] constexpr inline T bit_ceil(T x) noexcept {
    x -= 1;
    for (uint32_t i = 1; i < sizeof(x) * 8; ++i)
        if (1u << i > x)
            return 1u << i;
    return 0;
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

// C++20 - std::bit_cast
template <class To, class From>
[[nodiscard]] constexpr inline To bit_cast(const From& src) noexcept {
    static_assert(sizeof(To) == sizeof(From));
    static_assert(std::is_trivially_copyable_v<From>);
    static_assert(std::is_trivially_copyable_v<To>);
    static_assert(std::is_trivially_constructible_v<To>);
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

// C++20 - std::byteswap
template <typename T>
[[nodiscard]] constexpr inline T byteswap(T value) noexcept {
    static_assert(std::has_unique_object_representations_v<T>, "T may have padding bits");
    auto representation = bit_cast<std::array<std::byte, sizeof(T)>>(value);
    for (uint32_t i = 0; i < representation.size() / 2u; ++i)
        std::swap(representation[i], representation[representation.size() - 1 - i]);
    return bit_cast<T>(representation);
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

// --- Half utilities ------------------------------------------------------------------------------
// Based on https://gist.github.com/rygorous/eb3a019b99fdaa9c3064

union FP32 {
    uint32_t u;
    float f;
    struct P {
        uint32_t Mantissa : 23;
        uint32_t Exponent : 8;
        uint32_t Sign : 1;
    } p;
};

union FP16 {
    uint16_t u;
    struct P {
        uint16_t Mantissa : 10;
        uint16_t Exponent : 5;
        uint16_t Sign : 1;
    } p;
};

inline float half_to_float(uint16_t value) {
    FP16 h;
    h.u = value;
    static const FP32 magic = {113 << 23};
    static const uint32_t shifted_exp = 0x7c00 << 13; // exponent mask after shift
    FP32 o;

    o.u = (h.u & 0x7fff) << 13;     // exponent/mantissa bits
    uint32_t exp = shifted_exp & o.u;   // just the exponent
    o.u += (127 - 15) << 23;        // exponent adjust

    // handle exponent special cases
    if (exp == shifted_exp) // Inf/NaN?
        o.u += (128 - 16) << 23;    // extra exp adjust
    else if (exp == 0) { // Zero/Denormal?
        o.u += 1 << 23;             // extra exp adjust
        o.f -= magic.f;             // renormalize
    }

    o.u |= (h.u & 0x8000) << 16;    // sign bit
    return o.f;
}

inline uint16_t float_to_half(float value) {
    FP32 f;
    f.f = value;
    FP16 o = {0};

    // Based on ISPC reference code (with minor modifications)
    if (f.p.Exponent == 0) // Signed zero/denormal (which will underflow)
        o.p.Exponent = 0;
    else if (f.p.Exponent == 255) { // Inf or NaN (all exponent bits set)
        o.p.Exponent = 31;
        o.p.Mantissa = f.p.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
    } else { // Normalized number
        // Exponent unbias the single, then bias the halfp
        int newexp = f.p.Exponent - 127 + 15;
        if (newexp >= 31) // Overflow, return signed infinity
            o.p.Exponent = 31;
        else if (newexp <= 0) { // Underflow
            if ((14 - newexp) <= 24) { // Mantissa might be non-zero
                uint32_t mant = f.p.Mantissa | 0x800000; // Hidden 1 bit
                o.p.Mantissa = mant >> (14 - newexp);
                if ((mant >> (13 - newexp)) & 1) // Check for rounding
                    o.u++; // Round, might overflow into exp bit, but this is OK
            }
        } else {
            o.p.Exponent = newexp;
            o.p.Mantissa = f.p.Mantissa >> 13;
            if (f.p.Mantissa & 0x1000) // Check for rounding
                o.u++; // Round, might overflow to inf, this is OK
        }
    }

    o.p.Sign = f.p.Sign;
    return o.u;
}

// -------------------------------------------------------------------------------------------------

template <typename T>
[[nodiscard]] constexpr inline T extract_bits(const void* data, uint32_t offset, uint32_t numBits) {
    assert(numBits <= sizeof(T) * 8);

    const auto* source = static_cast<const uint8_t*>(data);
    std::array<uint8_t, sizeof(T)> target{0};

    for (uint32_t i = 0; i < numBits; ++i) {
        const auto sourceBitIndex = offset + i;
        const auto sourceByteIndex = sourceBitIndex / 8;
        const auto sourceBitSubByteIndex = sourceBitIndex % 8;
        const auto sourceBitSubByteMask = 1u << sourceBitSubByteIndex;
        const auto sourceBitValue = (source[sourceByteIndex] & sourceBitSubByteMask) != 0;
        const auto targetBitIndex = i;
        const auto targetByteIndex = targetBitIndex / 8;
        const auto targetBitSubByteIndex = targetBitIndex % 8;
        target[targetByteIndex] |= sourceBitValue ? 1u << targetBitSubByteIndex : 0u;
    }

    return bit_cast<T>(target);
}

[[nodiscard]] inline float covertSFloatToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits == 16 || numBits == 32);
    if (numBits == 16)
        return half_to_float(static_cast<uint16_t>(rawBits));
    if (numBits == 32)
        return bit_cast<float>(rawBits);
    return 0;
}
[[nodiscard]] inline float covertUFloatToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits == 10 || numBits == 11 || numBits == 14);
    // TODO: Tools P4: covertUFloatToFloat for 10, 11 and "14"
    (void) rawBits;
    (void) numBits;
    assert(false && "Not yet implemented");
    return 0;
}
[[nodiscard]] inline float covertSIntToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32);
    const auto signBit = (rawBits & 1u << (numBits - 1)) != 0;
    const auto valueBits = rawBits & ~(1u << (numBits - 1));
    const auto signedValue = static_cast<int32_t>(valueBits) * (signBit ? -1 : 1);
    return static_cast<float>(signedValue);
}
[[nodiscard]] inline float covertUIntToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32); (void) numBits;
    return static_cast<float>(rawBits);
}
[[nodiscard]] inline uint32_t covertSFloatToUInt(uint32_t rawBits, uint32_t numBits) {
    assert(numBits == 16 || numBits == 32);
    if (numBits == 16)
        return static_cast<uint32_t>(half_to_float(static_cast<uint16_t>(rawBits)));
    if (numBits == 32)
        return static_cast<uint32_t>(bit_cast<float>(rawBits));
    return 0;
}
[[nodiscard]] inline uint32_t covertUFloatToUInt(uint32_t rawBits, uint32_t numBits) {
    assert(numBits == 10 || numBits == 11 || numBits == 14);
    (void) rawBits;
    (void) numBits;
    assert(false && "Not yet implemented");
    return 0;
}
[[nodiscard]] inline uint32_t covertSIntToUInt(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32);
    (void) rawBits;
    (void) numBits;
    assert(false && "Not yet implemented");
    return 0;
}
[[nodiscard]] inline uint32_t covertUIntToUInt(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32);
    (void) numBits;
    return rawBits;
}

[[nodiscard]] constexpr inline uint32_t convertUNORM(uint32_t rawBits, uint32_t sourceBits, uint32_t targetBits) noexcept {
    assert(sourceBits > 0 && sourceBits <= 32);
    assert(targetBits > 0 && targetBits <= 32);

    rawBits &= (1u << sourceBits) - 1u;
    if (targetBits == sourceBits) {
        return rawBits;
    } else if (targetBits >= sourceBits) {
        // Upscale with "left bit replication" to fill in the least significant bits
        uint64_t result = 0;
        for (uint32_t i = 0; i < targetBits; i += sourceBits)
            result |= static_cast<uint64_t>(rawBits) << (targetBits - i) >> sourceBits;

        return static_cast<uint32_t>(result);
    } else {
        // Downscale with rounding: Check the most significant bit that was dropped: 1 -> up, 0 -> down
        const auto msDroppedBitIndex = sourceBits - targetBits - 1u;
        const auto msDroppedBitValue = rawBits & (1u << msDroppedBitIndex);
        if (msDroppedBitValue)
            // Min stops the 'overflow' if every targetBit is saturated and we would round up
            return std::min((rawBits >> (sourceBits - targetBits)) + 1u, (1u << targetBits) - 1u);
        else
            return rawBits >> (sourceBits - targetBits);
    }
}

[[nodiscard]] constexpr inline uint32_t convertUINT(uint32_t rawBits, uint32_t sourceBits, uint32_t targetBits) noexcept {
    assert(sourceBits > 0 && sourceBits <= 32);
    assert(targetBits > 0 && targetBits <= 32);

    const auto targetValueMask = targetBits == 32 ? std::numeric_limits<uint32_t>::max() : (1u << targetBits) - 1u;
    const auto sourceValueMask = sourceBits == 32 ? std::numeric_limits<uint32_t>::max() : (1u << sourceBits) - 1u;

    rawBits &= sourceValueMask;
    if (targetBits < sourceBits)
        rawBits &= targetValueMask;

    return rawBits;
}

[[nodiscard]] constexpr inline uint32_t convertSINT(uint32_t rawBits, uint32_t sourceBits, uint32_t targetBits) noexcept {
    assert(sourceBits > 1 && sourceBits <= 32);
    assert(targetBits > 1 && targetBits <= 32);

    const auto sourceSignBitIndex = sourceBits - 1u;
    const auto sourceSignMask = 1u << sourceSignBitIndex;
    const auto sign = (rawBits & sourceSignMask) != 0;
    const auto sourceValueBits = sourceBits - 1u;
    const auto sourceValueMask = (1u << sourceValueBits) - 1u;
    const auto sourceValue = rawBits & sourceValueMask;
    const auto targetSignBitIndex = targetBits - 1u;
    const auto targetValueBits = targetBits - 1u;
    const auto targetValueMask = (1u << targetValueBits) - 1u;

    uint32_t result = 0;
    result |= (sign ? 1u : 0u) << targetSignBitIndex;

    if (targetBits < sourceBits)
        result |= sourceValue & targetValueMask;
    else
        result |= sourceValue;

    return result;
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
