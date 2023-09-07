// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <array>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <glm/gtc/packing.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// -----------------------------------------------------------------------------

namespace imageio {

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

// C++20 - std::bit_ceil
template <typename T>
[[nodiscard]] constexpr inline T bit_ceil(T x) noexcept {
    x -= 1;
    for (uint32_t i = 0; i < sizeof(x) * 8; ++i)
        if (1u << i > x)
            return 1u << i;
    return 0;
}

// --- Half utilities ----------------------------------------------------------
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

// -----------------------------------------------------------------------------

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

[[nodiscard]] inline uint32_t convertFloatToUNORM(float value, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32);
    if (std::isnan(value))
        return 0;
    if (value < 0.f)
        return 0;
    if (value > 1.f)
        return (1u << numBits) - 1u;
    return static_cast<uint32_t>(value * static_cast<float>((1u << numBits) - 1u) + 0.5f);
}

[[nodiscard]] inline float convertSFloatToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits == 16 || numBits == 32);
    if (numBits == 16)
        return half_to_float(static_cast<uint16_t>(rawBits));
    if (numBits == 32)
        return bit_cast<float>(rawBits);
    return 0;
}
[[nodiscard]] inline float convertUFloatToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits == 10 || numBits == 11);
    if (numBits == 10)
        return glm::detail::packed10bitToFloat(rawBits);
    else if (numBits == 11)
        return glm::detail::packed11bitToFloat(rawBits);
    return 0;
}
[[nodiscard]] inline float convertSIntToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32);
    const auto signBit = (rawBits & 1u << (numBits - 1)) != 0;
    const auto valueBits = rawBits & ~(1u << (numBits - 1));
    const auto signedValue = static_cast<int32_t>(valueBits) * (signBit ? -1 : 1);
    return static_cast<float>(signedValue);
}
[[nodiscard]] inline float convertUIntToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32); (void) numBits;
    return static_cast<float>(rawBits);
}
[[nodiscard]] inline float convertSNORMToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32);
    (void) rawBits;
    (void) numBits;
    assert(false && "Not yet implemented");
    return 0;
}
[[nodiscard]] inline float convertUNORMToFloat(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32);
    const auto upper = static_cast<float>((1u << numBits) - 1u);
    return static_cast<float>(rawBits) / upper;
}
[[nodiscard]] inline uint32_t convertSFloatToUInt(uint32_t rawBits, uint32_t numBits) {
    assert(numBits == 16 || numBits == 32);
    if (numBits == 16)
        return static_cast<uint32_t>(half_to_float(static_cast<uint16_t>(rawBits)));
    if (numBits == 32)
        return static_cast<uint32_t>(bit_cast<float>(rawBits));
    return 0;
}
[[nodiscard]] inline uint32_t convertUFloatToUInt(uint32_t rawBits, uint32_t numBits) {
    assert(numBits == 10 || numBits == 11 || numBits == 14);
    (void) rawBits;
    (void) numBits;
    assert(false && "Not yet implemented");
    return 0;
}
[[nodiscard]] inline uint32_t convertSIntToUInt(uint32_t rawBits, uint32_t numBits) {
    assert(numBits > 0 && numBits <= 32);
    (void) rawBits;
    (void) numBits;
    assert(false && "Not yet implemented");
    return 0;
}
[[nodiscard]] inline uint32_t convertUIntToUInt(uint32_t rawBits, uint32_t numBits) {
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

} // namespace imageio
