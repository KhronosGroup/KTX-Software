// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2010-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file image.hpp
//!
//! @brief Internal Image class
//!

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <KHR/khr_df.h>
#include <fmt/format.h>
#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4201)
#endif
#include <glm/gtc/packing.hpp>
#ifdef _MSC_VER
  #pragma warning(pop)
#endif
#include "imageio_utility.h"
#include "unused.h"
#include "encoder/basisu_resampler.h"
#include "encoder/basisu_resampler_filters.h"

// cclamp to avoid conflict in toktx.cc with clamp template defined in scApp.
template <typename T> inline T cclamp(T value, T low, T high) {
    return (value < low) ? low : ((value > high) ? high : value);
}
template <typename T> inline T saturate(T value) {
    return cclamp<T>(value, 0, 1.0f);
}

template <typename S> inline S maximum(S a, S b) { return (a > b) ? a : b; }
template <typename S> inline S minimum(S a, S b) { return (a < b) ? a : b; }

#if defined(_MSC_VER)
#define INLINE __inline
#else
#define INLINE __inline__
#endif

struct ColorPrimaryTransform {
    ColorPrimaryTransform() {}

    ColorPrimaryTransform(const std::vector<float>& elements) {
        for (uint32_t i = 0; i < 3; ++i)
            for (uint32_t j = 0; j < 3; ++j)
                matrix[i][j] = elements[i * 3 + j];
    }

    float matrix[3][3];
};

// The detailed description of the TransferFunctions can be found at:
// https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#TRANSFER_CONVERSION

struct TransferFunction {
    virtual float encode(const float intensity) const = 0;
    virtual float decode(const float brightness) const = 0;
    virtual ~TransferFunction() {}
};

struct TransferFunctionLinear : public TransferFunction {
    float encode(const float intensity) const override {
        return intensity;
    }
    float decode(const float brightness) const override {
        return brightness;
    }
};

struct TransferFunctionGamma : public TransferFunction {
    TransferFunctionGamma(float oeGamma) : oeGamma_{oeGamma}, eoGamma_{1.f / oeGamma} {}

    float encode(const float intensity) const override {
        return saturate(powf(intensity, oeGamma_));
    }

    float decode(const float brightness) const override {
        return saturate(powf(brightness, eoGamma_));
    }

private:
    const float oeGamma_;
    const float eoGamma_;
};

struct TransferFunctionSRGB : public TransferFunction {
    float encode(const float intensity) const override {
        float brightness;

        if (intensity < 0.0031308f)
            brightness = 12.92f * intensity;
        else
            brightness = 1.055f * pow(intensity, 1.0f/2.4f) - 0.055f;

        return brightness;
    }

    float decode(const float brightness) const override {
        float intensity;

        if (brightness < .04045f)
            intensity = saturate(brightness * (1.0f/12.92f));
        else
            intensity = saturate(powf((brightness + .055f) * (1.0f/1.055f), 2.4f));

        return intensity;
    }
};

struct TransferFunctionITU : public TransferFunction {
    float encode(const float intensity) const override {
        float brightness;

        if (intensity < linearCutoff_)
            brightness = intensity * linearExpansion_;
        else
            brightness = 1.099f * pow(intensity, oeGamma_) - 0.099f;

        return brightness;
    }

    float decode(const float brightness) const override {
        float intensity;

        if (brightness < linearCutoff_ * linearExpansion_)
            intensity = brightness / linearExpansion_;
        else
            intensity = pow((brightness + 0.099f) / 1.099f, eoGamma_);

        return intensity;
    }

private:
    /* We're following what Netpbm does. This is their comment and code. */

    /* Here are parameters of the gamma transfer function for the Netpbm
    formats.  This is ITU-R Recommendation BT.709, FKA CIE Rec 709.  It is
    also ITU-R Recommendation BT.601, FKA CCIR 601.

    This transfer function is linear for sample values 0 .. .018
    and an exponential for larger sample values.
    The exponential is slightly stretched and translated, though,
    unlike the popular pure exponential gamma transfer function.

    The standard actually defines the linear expansion as 4.500, which
    means there is a discontinuity at linear intensity .018.  We instead
    use ~4.514 to make a continuous function.  This may have been simply
    a mistake when this code was written or based on an actual benefit
    to having a continuous function -- The history is not clear.

    Note that the discrepancy is below the precision of a maxval 255
    image.
    */
    const float eoGamma_{2.2f};
    const float oeGamma_{1.0f / eoGamma_};
    const float linearCutoff_{0.018f};
    const float linearExpansion_{(1.099f * pow(linearCutoff_, oeGamma_) - 0.099f) / linearCutoff_};
};

struct TransferFunctionBT2100_PQ_EOTF : public TransferFunction {
    float decode(const float brightness) const override {
        float Ym1 = pow(brightness, m1_);
        return pow((c1_ + c2_ * Ym1) / (1 + c3_ * Ym1), m2_);
    }

    float encode(const float intensity) const override {
        float Erm2 = pow(intensity, rm2_);
        return std::pow(std::max(Erm2 - c1_, 0.f) / (c2_ - c3_ * Erm2), m1_);
    }

private:
    const float m1_{0.1593017578125f};
    // const float rm1_{1.f / m1_}; // Commented to stop unused warning
    const float m2_{78.84375f};
    const float rm2_{1.f / m2_};
    const float c1_{0.8359375f};
    const float c2_{18.8515625f};
    const float c3_{18.6875};
};

// The detailed description of the ColorPrimaries can be found at:
// https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#PRIMARY_CONVERSION

struct ColorPrimaries {
    ColorPrimaries(const ColorPrimaryTransform& inToXYZ, const ColorPrimaryTransform& inFromXYZ)
        : toXYZ(inToXYZ), fromXYZ(inFromXYZ) {}

    ColorPrimaryTransform transformTo(const ColorPrimaries& targetPrimaries) const {
        ColorPrimaryTransform result;
        for (uint32_t i = 0; i < 3; ++i)
            for (uint32_t j = 0; j < 3; ++j) {
                result.matrix[i][j] = 0;
                for (uint32_t k = 0; k < 3; ++k)
                    result.matrix[i][j] += toXYZ.matrix[i][k] * targetPrimaries.fromXYZ.matrix[k][j];
            }
        return result;
    }

    const ColorPrimaryTransform toXYZ;
    const ColorPrimaryTransform fromXYZ;
};


struct ColorPrimariesBT709 : public ColorPrimaries {
    ColorPrimariesBT709() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.412391f, +0.357584f, +0.180481f,
            +0.212639f, +0.715169f, +0.072192f,
            +0.019331f, +0.119195f, +0.950532f
        }),
        ColorPrimaryTransform({
            +3.240970f, -1.537383f, -0.498611f,
            -0.969244f, +1.875968f, +0.041555f,
            +0.055630f, -0.203977f, +1.056972f
        })
    ) {}
};

struct ColorPrimariesBT601_625_EBU : public ColorPrimaries {
    ColorPrimariesBT601_625_EBU() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.430554f, +0.341550f, +0.178352f,
            +0.222004f, +0.706655f, +0.071341f,
            +0.020182f, +0.129553f, +0.939322f
        }),
        ColorPrimaryTransform({
            +3.063361f, -1.393390f, -0.475824f,
            -0.969244f, +1.875968f, +0.041555f,
            +0.067861f, -0.228799f, +1.069090f
        })
    ) {}
};

struct ColorPrimariesBT601_525_SMPTE : public ColorPrimaries {
    ColorPrimariesBT601_525_SMPTE() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.393521f, +0.365258f, +0.191677f,
            +0.212376f, +0.701060f, +0.086564f,
            +0.018739f, +0.111934f, +0.958385f
        }),
        ColorPrimaryTransform({
            +3.506003f, -1.739791f, -0.544058f,
            -1.069048f, +1.977779f, +0.035171f,
            +0.056307f, -0.196976f, +1.049952f
        })
    ) {}
};

struct ColorPrimariesBT2020 : public ColorPrimaries {
    ColorPrimariesBT2020() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.636958f, +0.144617f, +0.168881f,
            +0.262700f, +0.677998f, +0.059302f,
            +0.000000f, +0.028073f, +1.060985f
        }),
        ColorPrimaryTransform({
            +1.716651f, -0.355671f, -0.253366f,
            -0.666684f, +1.616481f, +0.015769f,
            +0.017640f, -0.042771f, +0.942103f
        })
    ) {}
};

struct ColorPrimariesCIEXYZ : public ColorPrimaries {
    ColorPrimariesCIEXYZ() : ColorPrimaries(
        ColorPrimaryTransform({
            1.f, 0.f, 0.f,
            0.f, 1.f, 0.f,
            0.f, 0.f, 1.f
        }),
        ColorPrimaryTransform({
            1.f, 0.f, 0.f,
            0.f, 1.f, 0.f,
            0.f, 0.f, 1.f
        })
    ) {}
};

struct ColorPrimariesACES : public ColorPrimaries {
    ColorPrimariesACES() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.9525523959f,  0.0000000000f, +0.0000936786f,
            +0.3439664498f, +0.7281660966f, -0.0721325464f,
             0.0000000000f,  0.0000000000f, +1.0088251844f
        }),
        ColorPrimaryTransform({
            +1.0498110175f,  0.0000000000f, -0.0000974845f,
            -0.4959030231f, +1.3733130458f, +0.0982400361f,
             0.0000000000f,  0.0000000000f, +0.9912520182f
        })
    ) {}
};

struct ColorPrimariesACEScc : public ColorPrimaries {
    ColorPrimariesACEScc() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.6624541811f, +0.1340042065f, +0.1561876870f,
            +0.2722287168f, +0.6740817658f, +0.0536895174f,
            -0.0055746495f, +0.0040607335f, +1.0103391003f
        }),
        ColorPrimaryTransform({
            +1.6410233797f, -0.3248032942f, -0.2464246952f,
            -0.6636628587f, +1.6153315917f, +0.0167563477f,
            +0.0117218943f, -0.0082844420f, +0.9883948585f
        })
    ) {}
};

struct ColorPrimariesNTSC1953 : public ColorPrimaries {
    ColorPrimariesNTSC1953() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.606993f, +0.173449f, +0.200571f,
            +0.298967f, +0.586421f, +0.114612f,
            +0.000000f, +0.066076f, +1.117469f
        }),
        ColorPrimaryTransform({
            +1.909675f, -0.532365f, -0.288161f,
            -0.984965f, +1.999777f, -0.028317f,
            +0.058241f, -0.118246f, +0.896554f
        })
    ) {}
};

struct ColorPrimariesPAL525 : public ColorPrimaries {
    ColorPrimariesPAL525() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.415394f, +0.354637f, +0.210677f,
            +0.224181f, +0.680675f, +0.095145f,
            +0.019781f, +0.108679f, +1.053387f
        }),
        ColorPrimaryTransform({
            +3.321392f, -1.648181f, -0.515410f,
            -1.101064f, +2.037011f, +0.036225f,
            +0.051228f, -0.179211f, +0.955260f
        })
    ) {}
};

struct ColorPrimariesDisplayP3 : public ColorPrimaries {
    ColorPrimariesDisplayP3() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.4865709486f, +0.2656676932f, +0.1982172852f,
            +0.2289745641f, +0.6917385218f, +0.0792869141f,
             0.0000000000f, +0.0451133819f, +1.0439441689f
        }),
        ColorPrimaryTransform({
            +2.4934969119f, -0.9313836179f, -0.4027107845f,
            -0.8294889696f, +1.7626640603f, +0.0236246858f,
            +0.0358458302f, -0.0761723893f, +0.9568845240f
        })
    ) {}
};

struct ColorPrimariesAdobeRGB : public ColorPrimaries {
    ColorPrimariesAdobeRGB() : ColorPrimaries(
        ColorPrimaryTransform({
            +0.5766690429f, +0.1855582379f, +0.1882286462f,
            +0.2973449753f, +0.6273635663f, +0.0752914585f,
            +0.0270313614f, +0.0706888525f, +0.9913375368f
        }),
        ColorPrimaryTransform({
            +2.0415879038f, -0.5650069743f, -0.3447313508f,
            -0.9692436363f, +1.8759675015f, +0.0415550574f,
            +0.0134442806f, -0.1183623922f, +1.0151749944f
        })
    ) {}
};

template <typename componentType, uint32_t componentCount>
class color_base {
public:
    constexpr static uint32_t getComponentCount() { return componentCount; }
    constexpr static uint32_t getComponentSize() { return sizeof(componentType); }
    constexpr static uint32_t getPixelSize() {
        return componentCount * sizeof(componentType);
    }
    constexpr static componentType one() {
        if (std::is_floating_point_v<componentType>)
            return componentType{1};
        else
            return std::numeric_limits<componentType>::max();
    }
    constexpr static float rcpOne() {
        if (std::is_floating_point_v<componentType>)
            return 1.f;
        else
            return 1.f / static_cast<float>(std::numeric_limits<componentType>::max());
    }
    constexpr static float halfUnit() {
        if (std::is_floating_point_v<componentType>)
            return 0.f;
        else
            return 0.5f / static_cast<float>(std::numeric_limits<componentType>::max());
    }
    constexpr static componentType min() {
        return std::numeric_limits<componentType>::min();
    }
    constexpr static componentType max() {
        return std::numeric_limits<componentType>::max();
    }
    componentType clamp(componentType value) {
        return (value < min()) ? min() : ((value > max()) ? max() : value);
    }
};

struct vec3_base {
    float r;
    float g;
    float b;
    vec3_base() : r(0.0f), g(0.0f), b(0.0f) {}
    vec3_base(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
    void base_normalize() {
        float len = r * r + g * g + b * b;
        len = sqrtf(len);
        if (len > 0.0f)
        {
            r /= len;
            g /= len;
            b /= len;
        }
    }
    void clamp(float _a, float _b) {
        r = cclamp(r, _a, _b);
        g = cclamp(g, _a, _b);
        b = cclamp(b, _a, _b);
    }
};

static constexpr float gc_m[5]={0.0f, 128.0f, 32768.0f, 0.0f, 2147483648.0f};
static constexpr uint32_t gc_s[5]={0, 255, 65535, 0, 4294967295};

template <typename componentType>
struct vec3 : public vec3_base {
    static constexpr uint32_t i = sizeof(componentType);

    explicit vec3(float _r) : vec3_base(_r, 0.0f, 0.0f) {}
    vec3(float _r, float _g) : vec3_base(_r, _g, 0.0f) {}
    vec3(float _r, float _g, float _b) : vec3_base(_r, _g, _b) {}
    void normalize() {
        // Zero normals in range [-1, 1] can't be normalized
        if (gc_m[i] == r && gc_m[i] == g && gc_m[i] == b) {
            return;
        } else {
            r = (float)(r / (double)gc_s[i]) * 2.0f - 1.0f;
            g = (float)(g / (double)gc_s[i]) * 2.0f - 1.0f;
            b = (float)(b / (double)gc_s[i]) * 2.0f - 1.0f;
            clamp(-1.0f, 1.0f);
            base_normalize();
            r = (std::floor((r + 1.0f) * (float)gc_s[i] * 0.5f + 0.5f));
            g = (std::floor((g + 1.0f) * (float)gc_s[i] * 0.5f + 0.5f));
            b = (std::floor((b + 1.0f) * (float)gc_s[i] * 0.5f + 0.5f));
            clamp(0.0f, (float)gc_s[i]);
        }
    }
};

template<>
struct vec3<float> : public vec3_base {
    explicit vec3(float _r) : vec3_base(_r, 0.0f, 0.0f) {}
    vec3(float _r, float _g) : vec3_base(_r, _g, 0.0f) {}
    vec3(float _r, float _g, float _b) : vec3_base(_r, _g, _b) {}
    void normalize(){
        base_normalize();
    }
};

template <typename componentType, uint32_t componentCount>
class color { };

template <typename componentType>
class color<componentType, 4> : public color_base<componentType, 4> {
  public:
    using value_type = componentType;
    union {
        componentType comps[4];

        struct {
            componentType r;
            componentType g;
            componentType b;
            componentType a;
        };
    };
    color() {}
    color(componentType _r, componentType _g, componentType _b,
          componentType _a) : r(_r), g(_g), b(_b), a(_a) {}
    const componentType& operator [](unsigned int i) const {
        if (i > 3) i = 3;
        return comps[i];
    }
    componentType& operator [](unsigned int i) {
        if (i > 3) i = 3;
        return comps[i];
    }
    template <typename T>
    void set(uint32_t i, T val) {
        if (i > 3) i = 3;
        comps[i] = color_base<componentType, 4>::clamp((componentType)val);
    }
    void set(uint32_t i, componentType val) {
        if (i > 3) i = 3;
        comps[i] = val;
    }
    constexpr uint32_t comps_count() const {
        return 4;
    }
    void normalize() {
       vec3<componentType> v((float)r, (float)g, (float)b);
       v.normalize();
       r = (componentType)v.r;
       g = (componentType)v.g;
       b = (componentType)v.b;
     }
 };

template <typename componentType>
class color<componentType, 3> : public color_base<componentType, 3> {
  public:
    using value_type = componentType;
    union {
       componentType comps[3];

       struct {
           componentType r;
           componentType g;
           componentType b;
       };
    };
    color() {}
    color(componentType _r, componentType _g, componentType _b) :
        r(_r), g(_g), b(_b) {}
    color(componentType _r, componentType _g, componentType _b, componentType) :
       r(_r), g(_g), b(_b) {}
    const componentType& operator [](unsigned int i) const {
        if (i > 2) i = 2;
        return comps[i];
    }
    componentType& operator [](unsigned int i) {
        if (i > 2) i = 2;
        return comps[i];
    }
    template <typename T>
    void set(uint32_t i, T val) {
        if (i > 2) i = 2;
        comps[i] = color_base<componentType, 3>::clamp((componentType)val);
    }
    void set(uint32_t i, componentType val) {
        if (i > 2) i = 2;
        comps[i] = val;
    }
    constexpr uint32_t comps_count() const {
        return 3;
    }
    void normalize() {
        vec3<componentType> v((float)r, (float)g, (float)b);
        v.normalize();
        r = (componentType)v.r;
        g = (componentType)v.g;
        b = (componentType)v.b;
    }
};

template <typename componentType>
class color<componentType, 2> : public color_base<componentType, 2> {
  public:
    using value_type = componentType;
    union {
        componentType comps[2];

        struct {
            componentType r;
            componentType g;
        };
    };
    color() {}
    color(componentType _r, componentType _g) :
       r(_r), g(_g) {}
    color(componentType _r, componentType _g, componentType, componentType) :
      r(_r), g(_g) {}
    const componentType& operator [](unsigned int i) const {
        if (i > 1) i = 1;
        return comps[i];
    }
    componentType& operator [](unsigned int i) {
        if (i > 1) i = 1;
        return comps[i];
    }
    template <typename T>
    void set(uint32_t i, T val) {
        if (i > 1) i = 1;
        comps[i] = color_base<componentType, 2>::clamp((componentType)val);
    }
    void set(uint32_t i, componentType val) {
        if (i > 1) i = 1;
        comps[i] = val;
    }
    constexpr uint32_t comps_count() const {
        return 2;
    }
    void normalize() {
        vec3<componentType> v((float)r, (float)g,
                              (float)gc_s[sizeof(componentType)] * 0.5f);
        v.normalize();
        r = (componentType)v.r;
        g = (componentType)v.g;
    }
};

template <typename componentType>
class color<componentType, 1> : public color_base<componentType, 1> {
  public:
    using value_type = componentType;
    union {
        componentType comps[1];

        struct {
            componentType r;
        };
    };
    color() {}
    color(componentType _r) :
       r(_r) {}
    color(componentType _r, componentType, componentType, componentType) :
       r(_r) {}
    const componentType& operator [](unsigned int i) const {
        if (i > 0) i = 0;
        return comps[i];
    }
    componentType& operator [](unsigned int i) {
        if (i > 0) i = 0;
        return comps[i];
    }
    template <typename T>
    void set(uint32_t i, T val) {
        if (i > 0) i = 0;
        comps[i] = color_base<componentType, 1>::clamp((componentType)val);
    }
    void set(uint32_t i, componentType val) {
        if (i > 0) i = 0;
        comps[i] = val;
    }
    constexpr uint32_t comps_count() const {
        return 1;
    }
    void normalize() {
        // Normalizing single channel image doesn't make much sense
        // Here I assume single channel color is (X, 0, 0, 0)
        if (r != 0)
            r = (componentType)gc_s[sizeof(componentType)];
    }
};

// Abstract base class for all Images.
class Image {
  public:
    class different_format : public std::runtime_error {
      public:
        different_format() : std::runtime_error("") { }
    };
    class invalid_file : public std::runtime_error {
      public:
        invalid_file(std::string error)
            : std::runtime_error("Invalid file: " + error) { }
    };

    virtual ~Image() { };

    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    uint32_t getPixelCount() const { return width * height; }
    khr_df_transfer_e getOetf() const { return oetf; }
    void setOetf(khr_df_transfer_e noetf) { this->oetf = noetf; }
    khr_df_primaries_e getPrimaries() const { return primaries; }
    void setPrimaries(khr_df_primaries_e nprimaries) {
        this->primaries = nprimaries;
    }

    virtual operator uint8_t*() = 0;

    virtual size_t getByteCount() const = 0;
    virtual uint32_t getPixelSize() const = 0;
    virtual uint32_t getComponentCount() const = 0;
    virtual uint32_t getComponentSize() const = 0;
    virtual Image* createImage(uint32_t width, uint32_t height) = 0;
    /// Should only be used if the stored image data is UNORM convertable (with optional significant bit count)
    virtual std::vector<uint8_t> getUNORM(uint32_t numChannels, uint32_t targetBits, uint32_t sBits = 0) const = 0;
    /// Should only be used if the stored image data is UNORM convertable (packed into a single word)
    virtual std::vector<uint8_t> getUNORMPacked(
            uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3) const = 0;
    /// Should only be used if the stored image data is SFloat convertable
    virtual std::vector<uint8_t> getSFloat(uint32_t numChannels, uint32_t targetBits) const = 0;
    /// Should only be used if the stored image data is UFloat convertable
    virtual std::vector<uint8_t> getB10G11R11() const = 0;
    /// Should only be used if the stored image data is UFloat convertable
    virtual std::vector<uint8_t> getE5B9G9R9() const = 0;
    /// Should only be used if the stored image data is UINT convertable
    virtual std::vector<uint8_t> getUINT(uint32_t numChannels, uint32_t targetBits) const = 0;
    /// Should only be used if the stored image data is SINT convertable
    virtual std::vector<uint8_t> getSINT(uint32_t numChannels, uint32_t targetBits) const = 0;
    /// Should only be used if the stored image data is UINT convertable
    virtual std::vector<uint8_t> getUINTPacked(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3) const = 0;
    /// Should only be used if the stored image data is SINT convertable
    virtual std::vector<uint8_t> getSINTPacked(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3) const = 0;
    virtual std::unique_ptr<Image> resample(uint32_t targetWidth, uint32_t targetHeight,
            const char* filter, float filterScale, basisu::Resampler::Boundary_Op wrapMode) = 0;
    virtual Image& yflip() = 0;
    virtual Image& transformColorSpace(const TransferFunction& decode, const TransferFunction& encode,
                                       const ColorPrimaryTransform* transformPrimaries = nullptr) = 0;
    virtual Image& normalize() = 0;
    virtual Image& swizzle(std::string_view swizzle) = 0;
    virtual Image& copyToR(Image&, std::string_view swizzle) = 0;
    virtual Image& copyToRG(Image&, std::string_view swizzle) = 0;
    virtual Image& copyToRGB(Image&, std::string_view swizzle) = 0;
    virtual Image& copyToRGBA(Image&, std::string_view swizzle) = 0;

  protected:
    Image() : Image(0, 0) { }
    Image(uint32_t w, uint32_t h)
            : width(w), height(h), oetf(KHR_DF_TRANSFER_UNSPECIFIED),
              primaries(KHR_DF_PRIMARIES_BT709) { }

    uint32_t width, height;  // In pixels
    khr_df_transfer_e oetf;
    khr_df_primaries_e primaries;
};

// Base class for template and specializations
template<typename componentType, uint32_t componentCount>
class ImageT : public Image {
  friend class ImageT<componentType, 1>;
  friend class ImageT<componentType, 2>;
  friend class ImageT<componentType, 3>;
  friend class ImageT<componentType, 4>;
  public:
    using Color = color<componentType, componentCount>;
    ImageT(uint32_t w, uint32_t h) : Image(w, h)
    {
        size_t bytes = sizeof(Color) * w * h;
        pixels = (Color*)malloc(bytes);
        if (!pixels)
            throw std::bad_alloc();

        freePixels = true;

        for (uint32_t p = 0; p < w * h; ++p)
            for(uint32_t c = 0; c < componentCount; ++c)
                pixels[p].comps[c] = componentType{0};
    }

    ImageT(uint32_t w, uint32_t h, Color* pixels) : Image(w, h), pixels(pixels), freePixels(false)
    {
    }

    ~ImageT()
    {
        if (freePixels)
            free(pixels);
    }

    const Color& operator() (uint32_t x, uint32_t y) const {
       assert(x < width && y < height); return pixels[x + y * width];
    }
    Color& operator() (uint32_t x, uint32_t y) {
        assert(x < width && y < height); return pixels[x + y * width];
    }
    virtual operator uint8_t*() override { return (uint8_t*)pixels; }

    virtual size_t getByteCount() const override {
        return getPixelCount() * sizeof(Color);
    }

    virtual uint32_t getPixelSize() const override {
        return Color::getPixelSize();
    }
    virtual uint32_t getComponentCount() const override {
        return Color::getComponentCount();
    }
    virtual uint32_t getComponentSize() const override {
        return Color::getComponentSize();
    }

    virtual Image* createImage(uint32_t w, uint32_t h) override {
        ImageT* image = new ImageT(w, h);
        return image;
    }

    virtual std::vector<uint8_t> getUNORM(uint32_t numChannels, uint32_t targetBits, uint32_t sBits) const override {
        assert(numChannels <= componentCount);
        assert(targetBits == 8 || targetBits == 16 || targetBits == 32);

        const uint32_t sourceBits = sizeof(componentType) * 8;
        const uint32_t targetBytes = targetBits / 8;
        const uint32_t mask = (sBits == 0) ? 0xFFFFFFFF : ((1u << sBits) - 1u) << (targetBits - sBits);
        std::vector<uint8_t> data(height * width * numChannels * targetBytes);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                for (uint32_t c = 0; c < numChannels; ++c) {
                    const auto sourceValue = c < componentCount ? pixels[y * width + x][c] : (c != 3 ? componentType{0} : Color::one());
                    const auto value = imageio::convertUNORM(static_cast<uint32_t>(sourceValue), sourceBits, targetBits);
                    auto* target = data.data() + (y * width * numChannels + x * numChannels + c) * targetBytes;

                    if (targetBytes == 1) {
                        const auto outValue = static_cast<uint8_t>(value & mask);
                        std::memcpy(target, &outValue, sizeof(outValue));
                    } else if (targetBytes == 2) {
                        const auto outValue = static_cast<uint16_t>(value & mask);
                        std::memcpy(target, &outValue, sizeof(outValue));
                    } else if (targetBytes == 4) {
                        const auto outValue = static_cast<uint32_t>(value & mask);
                        std::memcpy(target, &outValue, sizeof(outValue));
                    }
                }
            }
        }

        return data;
    }

    virtual std::vector<uint8_t> getUNORMPacked(
            uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3) const override {
        assert((c0 + c1 + c2 + c3) % 8 == 0);
        const auto targetPackBytes = (c0 + c1 + c2 + c3) / 8;
        assert(targetPackBytes == 1 || targetPackBytes == 2 || targetPackBytes == 4 || targetPackBytes == 8);
        const auto packC0 = c0 > 0;
        const auto packC1 = c1 > 0;
        const auto packC2 = c2 > 0;
        const auto packC3 = c3 > 0;
        const auto numChannels = (packC0 ? 1u : 0) + (packC1 ? 1u : 0) + (packC2 ? 1u : 0) + (packC3 ? 1u : 0);
        assert(numChannels <= componentCount); (void) numChannels;
        const uint32_t sourceBits = sizeof(componentType) * 8;
        static constexpr auto hasC0 = componentCount > 0;
        static constexpr auto hasC1 = componentCount > 1;
        static constexpr auto hasC2 = componentCount > 2;
        static constexpr auto hasC3 = componentCount > 3;

        std::vector<uint8_t> data(height * width * targetPackBytes);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const auto& pixel = pixels[y * width + x];
                auto* target = data.data() + (y * width + x) * targetPackBytes;

                const auto copy = [&](auto& pack) {
                    using PackType = std::remove_reference_t<decltype(pack)>;

                    if (packC0) {
                        const auto sourceValue = hasC0 ? pixel[0] : componentType{0};
                        const auto value = imageio::convertUNORM(static_cast<uint32_t>(sourceValue), sourceBits, c0);
                        pack |= static_cast<PackType>(value) << (c1 + c2 + c3);
                    }
                    if (packC1) {
                        const auto sourceValue = hasC1 ? pixel[1] : componentType{0};
                        const auto value = imageio::convertUNORM(static_cast<uint32_t>(sourceValue), sourceBits, c1);
                        pack |= static_cast<PackType>(value) << (c2 + c3);
                    }
                    if (packC2) {
                        const auto sourceValue = hasC2 ? pixel[2] : componentType{0};
                        const auto value = imageio::convertUNORM(static_cast<uint32_t>(sourceValue), sourceBits, c2);
                        pack |= static_cast<PackType>(value) << c3;
                    }
                    if (packC3) {
                        const auto sourceValue = hasC3 ? pixel[3] : Color::one();
                        const auto value = imageio::convertUNORM(static_cast<uint32_t>(sourceValue), sourceBits, c3);
                        pack |= static_cast<PackType>(value);
                    }
                };

                if (targetPackBytes == 1) {
                    uint8_t pack = 0;
                    copy(pack);
                    std::memcpy(target, &pack, sizeof(pack));
                } else if (targetPackBytes == 2) {
                    uint16_t pack = 0;
                    copy(pack);
                    std::memcpy(target, &pack, sizeof(pack));
                } else if (targetPackBytes == 4) {
                    uint32_t pack = 0;
                    copy(pack);
                    std::memcpy(target, &pack, sizeof(pack));
                } else if (targetPackBytes == 8) {
                    uint64_t pack = 0;
                    copy(pack);
                    std::memcpy(target, &pack, sizeof(pack));
                }
            }
        }

        return data;
    }

    virtual std::vector<uint8_t> getSFloat(uint32_t numChannels, uint32_t targetBits) const override {
        assert(numChannels <= componentCount);
        assert(targetBits == 16 || targetBits == 32);

        const uint32_t targetBytes = targetBits / 8;
        std::vector<uint8_t> data(height * width * numChannels * targetBytes);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                for (uint32_t c = 0; c < numChannels; ++c) {
                    const auto value = c < componentCount ? pixels[y * width + x][c] : (c != 3 ? componentType{0} : componentType{1});
                    auto* target = data.data() + (y * width * numChannels + x * numChannels + c) * targetBytes;

                    if (sizeof(componentType) == targetBytes) {
                        *reinterpret_cast<componentType*>(target) = value;
                    } else if (targetBytes == 2) {
                        const auto outValue = imageio::float_to_half(static_cast<float>(value));
                        std::memcpy(target, &outValue, targetBytes);
                    } else if (targetBytes == 4) {
                        const auto outValue = static_cast<float>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    }
                }
            }
        }

        return data;
    }

    virtual std::vector<uint8_t> getB10G11R11() const override {
        assert(3 <= componentCount);
        assert(std::is_floating_point_v<componentType>);

        std::vector<uint8_t> data(height * width * 4);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                auto* target = data.data() + (y * width + x) * 4;

                const auto pixel = (*this)(x, y);
                const auto r = pixel[0];
                const auto g = pixel[1];
                const auto b = pixel[2];
                const auto outValue = glm::packF2x11_1x10(glm::vec3(r, g, b));
                std::memcpy(target, &outValue, sizeof(outValue));
            }
        }

        return data;
    }

    virtual std::vector<uint8_t> getE5B9G9R9() const override {
        assert(3 <= componentCount);
        assert(std::is_floating_point_v<componentType>);

        std::vector<uint8_t> data(height * width * 4);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                auto* target = data.data() + (y * width + x) * 4;

                const auto pixel = (*this)(x, y);
                const auto r = pixel[0];
                const auto g = pixel[1];
                const auto b = pixel[2];
                const auto outValue = glm::packF3x9_E1x5(glm::vec3(r, g, b));
                std::memcpy(target, &outValue, sizeof(outValue));
            }
        }

        return data;
    }

    virtual std::vector<uint8_t> getUINT(uint32_t numChannels, uint32_t targetBits) const override {
        assert(numChannels <= componentCount);
        assert(targetBits == 8 || targetBits == 16 || targetBits == 32 || targetBits == 64);

        const uint32_t targetBytes = targetBits / 8;
        std::vector<uint8_t> data(height * width * numChannels * targetBytes);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                for (uint32_t c = 0; c < numChannels; ++c) {
                    const auto value = c < componentCount ? pixels[y * width + x][c] : c != 3 ? 0 : componentType{1};
                    auto* target = data.data() + (y * width * numChannels + x * numChannels + c) * targetBytes;

                    if (targetBytes == 1) {
                        const auto outValue = static_cast<uint8_t>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    } else if (targetBytes == 2) {
                        const auto outValue = static_cast<uint16_t>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    } else if (targetBytes == 4) {
                        const auto outValue = static_cast<uint32_t>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    } else if (targetBytes == 8) {
                        const auto outValue = static_cast<uint64_t>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    }
                }
            }
        }

        return data;
    }

    virtual std::vector<uint8_t> getSINT(uint32_t numChannels, uint32_t targetBits) const override {
        assert(numChannels <= componentCount);
        assert(targetBits == 8 || targetBits == 16 || targetBits == 32 || targetBits == 64);

        const uint32_t targetBytes = targetBits / 8;
        std::vector<uint8_t> data(height * width * numChannels * targetBytes);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                for (uint32_t c = 0; c < numChannels; ++c) {
                    const auto value = c < componentCount ? pixels[y * width + x][c] : c != 3 ? 0 : componentType{1};
                    auto* target = data.data() + (y * width * numChannels + x * numChannels + c) * targetBytes;

                    if (targetBytes == 1) {
                        const auto outValue = static_cast<int8_t>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    } else if (targetBytes == 2) {
                        const auto outValue = static_cast<int16_t>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    } else if (targetBytes == 4) {
                        const auto outValue = static_cast<int32_t>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    } else if (targetBytes == 8) {
                        const auto outValue = static_cast<int64_t>(value);
                        std::memcpy(target, &outValue, targetBytes);
                    }
                }
            }
        }

        return data;
    }

    virtual std::vector<uint8_t> getUINTPacked(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3) const override {
        assert(c0 + c1 + c2 + c3 == 32);
        assert(c0 != 0 && c1 != 0 && c2 != 0 && c3 != 0);
        assert(componentCount == 4);

        std::vector<uint8_t> data(height * width * sizeof(uint32_t));
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const auto& pixel = pixels[y * width + x];
                auto* target = data.data() + (y * width + x) * sizeof(uint32_t);

                uint32_t pack = 0;
                pack |= imageio::convertUINT(static_cast<uint32_t>(pixel[0]), sizeof(uint32_t) * 8, c0) << (c1 + c2 + c3);
                pack |= imageio::convertUINT(static_cast<uint32_t>(pixel[1]), sizeof(uint32_t) * 8, c1) << (c2 + c3);
                pack |= imageio::convertUINT(static_cast<uint32_t>(pixel[2]), sizeof(uint32_t) * 8, c2) << c3;
                pack |= imageio::convertUINT(static_cast<uint32_t>(pixel[3]), sizeof(uint32_t) * 8, c3);

                std::memcpy(target, &pack, sizeof(pack));
            }
        }

        return data;
    }

    virtual std::vector<uint8_t> getSINTPacked(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3) const override {
        assert(c0 + c1 + c2 + c3 == 32);
        assert(c0 != 0 && c1 != 0 && c2 != 0 && c3 != 0);
        assert(componentCount == 4);

        std::vector<uint8_t> data(height * width * sizeof(uint32_t));
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const auto& pixel = pixels[y * width + x];
                auto* target = data.data() + (y * width + x) * sizeof(uint32_t);

                uint32_t pack = 0;
                pack |= imageio::convertSINT(imageio::bit_cast<uint32_t>(static_cast<int32_t>(pixel[0])), sizeof(uint32_t) * 8, c0) << (c1 + c2 + c3);
                pack |= imageio::convertSINT(imageio::bit_cast<uint32_t>(static_cast<int32_t>(pixel[1])), sizeof(uint32_t) * 8, c1) << (c2 + c3);
                pack |= imageio::convertSINT(imageio::bit_cast<uint32_t>(static_cast<int32_t>(pixel[2])), sizeof(uint32_t) * 8, c2) << c3;
                pack |= imageio::convertSINT(imageio::bit_cast<uint32_t>(static_cast<int32_t>(pixel[3])), sizeof(uint32_t) * 8, c3);

                std::memcpy(target, &pack, sizeof(pack));
            }
        }

        return data;
    }

    static void checkResamplerStatus(basisu::Resampler& resampler, const char* pFilter) {
        using Status = basisu::Resampler::Status;

        switch (resampler.status()) {
        case Status::STATUS_OKAY:
            break;
        case Status::STATUS_OUT_OF_MEMORY:
            throw std::runtime_error("Resampler or Resampler::put_line out of memory.");
        case Status::STATUS_BAD_FILTER_NAME:
            throw std::runtime_error(fmt::format("Unknown filter: {}", pFilter));
        case Status::STATUS_SCAN_BUFFER_FULL:
            throw std::runtime_error("Resampler::put_line scan buffer full.");
        }
    }

    virtual std::unique_ptr<Image> resample(uint32_t targetWidth, uint32_t targetHeight,
            const char* filter, float filterScale, basisu::Resampler::Boundary_Op wrapMode) override {
        using namespace basisu;

        auto target = std::make_unique<ImageT<componentType, componentCount>>(targetWidth, targetHeight);
        target->setOetf(oetf);
        target->setPrimaries(primaries);

        const auto sourceWidth = width;
        const auto sourceHeight = height;
        assert(sourceWidth && sourceHeight && targetWidth && targetHeight);

        if (std::max(sourceWidth, sourceHeight) > BASISU_RESAMPLER_MAX_DIMENSION ||
                std::max(targetWidth, targetHeight) > BASISU_RESAMPLER_MAX_DIMENSION) {
            throw std::runtime_error(fmt::format(
                    "Image larger than max supported size of {}", BASISU_RESAMPLER_MAX_DIMENSION));
        }

        std::array<std::vector<float>, componentCount> samples;
        std::array<std::unique_ptr<Resampler>, componentCount> resamplers;

        // Float types handled as SFloat HDR otherwise UNROM LDR is assumed
        const auto isHDR = std::is_floating_point_v<componentType>;

        for (uint32_t i = 0; i < componentCount; ++i) {
            resamplers[i] = std::make_unique<Resampler>(
                    sourceWidth, sourceHeight,
                    targetWidth, targetHeight,
                    wrapMode,
                    0.0f, isHDR ? 0.0f : 1.0f,
                    filter,
                    i == 0 ? nullptr : resamplers[0]->get_clist_x(),
                    i == 0 ? nullptr : resamplers[0]->get_clist_y(),
                    filterScale, filterScale,
                    0.f, 0.f);
            checkResamplerStatus(*resamplers[i], filter);
            samples[i].resize(sourceWidth);
        }

        const TransferFunctionSRGB tfSRGB;
        const TransferFunctionLinear tfLinear;
        const TransferFunction& tf = oetf == KHR_DF_TRANSFER_SRGB ?
                static_cast<const TransferFunction&>(tfSRGB) :
                static_cast<const TransferFunction&>(tfLinear);

        uint32_t targetY = 0;
        for (uint32_t sourceY = 0; sourceY < sourceHeight; ++sourceY) {
            // Put source lines into resampler(s)
            for (uint32_t sourceX = 0; sourceX < sourceWidth; ++sourceX) {
                const auto& sourcePixel = pixels[sourceY * sourceWidth + sourceX];
                for (uint32_t c = 0; c < componentCount; ++c) {
                    const float value = std::is_floating_point_v<componentType> ?
                            sourcePixel[c] :
                            static_cast<float>(sourcePixel[c]) * (1.f / static_cast<float>(Color::one()));

                    // c == 3: Alpha channel always uses tfLinear
                    samples[c][sourceX] = (c == 3 ? tfLinear : tf).decode(value);
                }
            }

            for (uint32_t c = 0; c < componentCount; ++c)
                if (!resamplers[c]->put_line(&samples[c][0]))
                    checkResamplerStatus(*resamplers[c], filter);

            // Retrieve any output lines
            while (true) {
                std::array<const float*, componentCount> outputLine{nullptr};
                for (uint32_t c = 0; c < componentCount; ++c)
                    outputLine[c] = resamplers[c]->get_line();

                if (outputLine[0] == nullptr)
                    break; // No new output line, break from retrieve and place in a new source line

                for (uint32_t targetX = 0; targetX < targetWidth; ++targetX) {
                    Color& targetPixel = target->pixels[targetY * targetWidth + targetX];
                    for (uint32_t c = 0; c < componentCount; ++c) {
                        const auto linearValue = outputLine[c][targetX];

                        // c == 3: Alpha channel always uses tfLinear
                        const float outValue = (c == 3 ? tfLinear : tf).encode(linearValue);
                        if constexpr (std::is_floating_point_v<componentType>) {
                            targetPixel[c] = outValue;
                        } else {
                            const auto unormValue =
                                std::isnan(outValue) ? componentType{0} :
                                outValue < 0.f ? componentType{0} :
                                outValue > 1.f ? Color::one() :
                                static_cast<componentType>(outValue * static_cast<float>(Color::one()) + 0.5f);
                            targetPixel[c] = unormValue;
                        }
                    }
                }

                ++targetY;
            }
        }

        return target;
    }

    virtual ImageT& yflip() override {
        uint32_t rowSize = width * sizeof(Color);
        // Minimize memory use by only buffering a single row.
        Color* rowBuffer = new Color[width];

        for (uint32_t sy = height-1, dy = 0; sy >= height / 2; sy--, dy++) {
            Color* srcRow = &pixels[width * sy];
            Color* dstRow = &pixels[width * dy];

            memcpy(rowBuffer, dstRow, rowSize);
            memcpy(dstRow, srcRow, rowSize);
            memcpy(srcRow, rowBuffer, rowSize);
        }
        delete[] rowBuffer;
        return *this;
    }

    virtual ImageT& transformColorSpace(const TransferFunction& decode, const TransferFunction& encode,
                                        const ColorPrimaryTransform* transformPrimaries) override {
        uint32_t pixelCount = getPixelCount();
        for (uint32_t i = 0; i < pixelCount; ++i) {
            Color& c = pixels[i];
            // Don't transform the alpha component.
            uint32_t components = cclamp(getComponentCount(), 0u, 3u);
            float intensity[3];
            float brightness[3];

            // Decode source transfer function
            for (uint32_t comp = 0; comp < components; comp++) {
                brightness[comp] = (float)(c[comp]) * Color::rcpOne();
                intensity[comp] = decode.decode(brightness[comp]);
            }

            // If needed, transform primaries
            if (transformPrimaries != nullptr) {
                float origIntensity[3] = { intensity[0], intensity[1], intensity[2] };
                for (uint32_t j = 0; j < components; ++j) {
                    intensity[j] = 0.f;
                    for (uint32_t k = 0; k < components; ++k)
                        intensity[j] += transformPrimaries->matrix[j][k] * origIntensity[k];
                }
            }

            // Encode destination transfer function
            for (uint32_t comp = 0; comp < components; comp++) {
                brightness[comp] = encode.encode(intensity[comp]);
                // clamp(value, color::min, color::max) is required as static_cast has platform-specific behaviors
                // and on certain platforms can over or underflow
                c.set(comp, cclamp(
                        roundf(brightness[comp] * static_cast<float>(Color::one())),
                        static_cast<float>(Color::min()),
                        static_cast<float>(Color::max())));
            }
        }
        return *this;
    }

    virtual ImageT& normalize() override {
        uint32_t pixelCount = getPixelCount();
        for (uint32_t i = 0; i < pixelCount; ++i) {
            Color& c = pixels[i];
            c.normalize();
        }
        return *this;
    }

    virtual ImageT& swizzle(std::string_view swizzle) override {
        assert(swizzle.size() == 4);
        for (size_t i = 0; i < getPixelCount(); i++) {
            Color srcPixel = pixels[i];
            for (uint32_t c = 0; c < getComponentCount(); c++) {
                pixels[i].set(c, swizzlePixel(srcPixel, swizzle[c]));
            }
        }
        return *this;
    }

    template<class DstImage>
    ImageT& copyTo(DstImage& dst, std::string_view swizzle) {
        assert(getComponentSize() == dst.getComponentSize());
        assert(width == dst.getWidth() && height == dst.getHeight());

        dst.setOetf(oetf);
        dst.setPrimaries(primaries);
        for (size_t i = 0; i < getPixelCount(); i++) {
            uint32_t c;
            for (c = 0; c < dst.getComponentCount(); c++) {
                if (c < getComponentCount())
                    dst.pixels[i].set(c, swizzlePixel(pixels[i], swizzle[c]));
                else
                    break;
            }
            for (; c < dst.getComponentCount(); c++)
                if (c < 3)
                    dst.pixels[i].set(c, componentType{0});
                else
                    dst.pixels[i].set(c, Color::one());
        }
        return *this;
    }

    virtual ImageT& copyToR(Image& dst, std::string_view swizzle) override { return copyTo((ImageT<componentType, 1>&)dst, swizzle); }
    virtual ImageT& copyToRG(Image& dst, std::string_view swizzle) override { return copyTo((ImageT<componentType, 2>&)dst, swizzle); }
    virtual ImageT& copyToRGB(Image& dst, std::string_view swizzle) override { return copyTo((ImageT<componentType, 3>&)dst, swizzle); }
    virtual ImageT& copyToRGBA(Image& dst, std::string_view swizzle) override { return copyTo((ImageT<componentType, 4>&)dst, swizzle); }

  protected:
    componentType swizzlePixel(const Color& srcPixel, char swizzle) {
        switch (swizzle) {
          case 'r':
            return srcPixel[0];
          case 'g':
            return srcPixel[1];
          case 'b':
            return srcPixel[2];
          case 'a':
            return srcPixel[3];
          case '0':
            return componentType{0};
          case '1':
            return Color::one();
          default:
            assert(false);
            return componentType{0};
        }
    }

    Color* pixels;
    bool freePixels;
};

using r8color = color<uint8_t, 1>;
using rg8color = color<uint8_t, 2>;
using rgb8color = color<uint8_t, 3>;
using rgba8color = color<uint8_t, 4>;
using r16color = color<uint16_t, 1>;
using rg16color = color<uint16_t, 2>;
using rgb16color = color<uint16_t, 3>;
using rgba16color = color<uint16_t, 4>;
using r32color = color<uint32_t, 1>;
using rg32color = color<uint32_t, 2>;
using rgb32color = color<uint32_t, 3>;
using rgba32color = color<uint32_t, 4>;

using r8scolor = color<int8_t, 1>;
using rg8scolor = color<int8_t, 2>;
using rgb8scolor = color<int8_t, 3>;
using rgba8scolor = color<int8_t, 4>;
using r16scolor = color<int16_t, 1>;
using rg16scolor = color<int16_t, 2>;
using rgb16scolor = color<int16_t, 3>;
using rgba16scolor = color<int16_t, 4>;
using r32scolor = color<int32_t, 1>;
using rg32scolor = color<int32_t, 2>;
using rgb32scolor = color<int32_t, 3>;
using rgba32scolor = color<int32_t, 4>;

using r32fcolor = color<float, 1>;
using rg32fcolor = color<float, 2>;
using rgb32fcolor = color<float, 3>;
using rgba32fcolor = color<float, 4>;

using r8image = ImageT<uint8_t, 1>;
using rg8image = ImageT<uint8_t, 2>;
using rgb8image = ImageT<uint8_t, 3>;
using rgba8image = ImageT<uint8_t, 4>;
using r16image = ImageT<uint16_t, 1>;
using rg16image = ImageT<uint16_t, 2>;
using rgb16image = ImageT<uint16_t, 3>;
using rgba16image = ImageT<uint16_t, 4>;
using r32image = ImageT<uint32_t, 1>;
using rg32image = ImageT<uint32_t, 2>;
using rgb32image = ImageT<uint32_t, 3>;
using rgba32image = ImageT<uint32_t, 4>;

using r8simage = ImageT<int8_t, 1>;
using rg8simage = ImageT<int8_t, 2>;
using rgb8simage = ImageT<int8_t, 3>;
using rgba8simage = ImageT<int8_t, 4>;
using r16simage = ImageT<int16_t, 1>;
using rg16simage = ImageT<int16_t, 2>;
using rgb16simage = ImageT<int16_t, 3>;
using rgba16simage = ImageT<int16_t, 4>;
using r32simage = ImageT<int32_t, 1>;
using rg32simage = ImageT<int32_t, 2>;
using rgb32simage = ImageT<int32_t, 3>;
using rgba32simage = ImageT<int32_t, 4>;

using r32fimage = ImageT<float, 1>;
using rg32fimage = ImageT<float, 2>;
using rgb32fimage = ImageT<float, 3>;
using rgba32fimage = ImageT<float, 4>;

#endif /* IMAGE_HPP */
