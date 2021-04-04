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

#include <math.h>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <KHR/khr_df.h>

#include "argparser.h"
#include "encoder/basisu_resampler.h"
#include "encoder/basisu_resampler_filters.h"

typedef float (*OETFFunc)(float const, float const);

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

static INLINE float
encode_bt709(float const intensity, float const) {
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
    float const gamma = 2.2f;
    float const oneOverGamma = 1.0f / gamma;
    float const linearCutoff = 0.018f;
    float const linearExpansion =
        (1.099f * pow(linearCutoff, oneOverGamma) - 0.099f) / linearCutoff;

    float brightness;

    if (intensity < linearCutoff)
        brightness = intensity * linearExpansion;
    else
        brightness = 1.099f * pow(intensity, oneOverGamma) - 0.099f;

    return brightness;
}

static INLINE float
decode_bt709(float const brightness, float const)
{
    float const gamma = 2.2f;
    float const oneOverGamma = 1.0f / gamma;
    float const linearCutoff = 0.018f;
    float const linearExpansion =
        (1.099f * pow(linearCutoff, oneOverGamma) - 0.099f) / linearCutoff;

    float intensity;

    if (brightness < linearCutoff * linearExpansion)
        intensity = brightness / linearExpansion;
    else
        intensity = pow((brightness + 0.099f) / 1.099f, gamma);

    return intensity;
}

// These are called from resample as well as decode, hence the default
// parameter values.
static INLINE float
encode_sRGB(float const intensity, float const unused = 1.0f)
{
    float brightness;
    if (intensity < 0.0031308f)
        brightness = 12.92f * intensity;
    else
        brightness = 1.055f * pow(intensity, 1.0f/2.4f) - 0.055f;

    return brightness;
}

static INLINE float
decode_sRGB(float const brightness, float const unused = 1.0f)
{
    float intensity;

    if (brightness < .04045f)
        intensity = saturate(brightness * (1.0f/12.92f));
    else
        intensity = saturate(powf((brightness + .055f) * (1.0f/1.055f), 2.4f));

    return intensity;
}

static INLINE float
decode_gamma(float const brightness, float const gamma)
{
    return saturate(powf(brightness, gamma));
}

static INLINE float
decode_linear(float const brightness, float const)
{
    return brightness;
}

static INLINE float
encode_linear(float const intensity, float const)
{
    return intensity;
}

template <typename componentType, uint32_t componentCount>
class color_base {
public:
    static uint32_t getComponentCount() { return componentCount; }
    static uint32_t getComponentSize() { return sizeof(componentType); }
    static uint32_t getPixelSize() { return componentCount * sizeof(componentType); }
    static uint32_t one() { return ((1 << sizeof(componentType) * 8) - 1); }
};

template <typename componentType, uint32_t componentCount>
class color { };

template <typename componentType>
class color<componentType, 4> : public color_base<componentType, 4> {
  public:
     union {
         componentType comps[4];

         struct {
             componentType r;
             componentType g;
             componentType b;
             componentType a;
         };
     };
     componentType operator [](unsigned int i) {
         if (i > 3) i = 3;
         return comps[i];
     }
     void set(uint32_t i, float val) {
         if (i > 3) i = 3;
         comps[i] = (componentType)val;
     }
     void set(uint32_t i, componentType val) {
         if (i > 3) i = 3;
         comps[i] = val;
     }
};

template <typename componentType>
class color<componentType, 3> : public color_base<componentType, 3> {
  public:
    union {
       componentType comps[3];

       struct {
           componentType r;
           componentType g;
           componentType b;
       };
    };
    componentType operator [](unsigned int i) {
        if (i > 2) i = 2;
        return comps[i];
    }
    void set(uint32_t i, float val) {
        if (i > 2) i = 2;
        comps[i] = (componentType)val;
    }
};

template <typename componentType>
class color<componentType, 2> : public color_base<componentType, 2> {
  public:
     union {
         componentType comps[2];

         struct {
             componentType r;
             componentType g;
         };
     };
     componentType operator [](unsigned int i) {
         if (i > 1) i = 1;
         return comps[i];
     }
     void set(uint32_t i, float val) {
         if (i > 1) i = 1;
         comps[i] = (componentType)val;
     }
};

template <typename componentType>
class color<componentType, 1> : public color_base<componentType, 1> {
  public:
     union {
         componentType comps[1];

         struct {
             componentType r;
         };
     };
     componentType operator [](unsigned int i) {
         if (i > 0) i = 0;
         return comps[i];
     }
     void set(uint32_t i, float val) {
         if (i > 0) i = 0;
         comps[i] = (componentType)val;
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
    enum colortype_e {
        eLuminance=0, eLuminanceAlpha=1, eR=2, eRG, eRGB, eRGBA
    };

    virtual ~Image() { };

    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    uint32_t getPixelCount() const { return width * height; }
    colortype_e getColortype() { return this->colortype; }
    void setColortype(colortype_e t) { colortype = t; }
    khr_df_transfer_e getOetf() const { return oetf; }
    void setOetf(khr_df_transfer_e oetf) { this->oetf = oetf; }
    khr_df_primaries_e getPrimaries() const { return primaries; }
    void setPrimaries(khr_df_primaries_e primaries) {
        this->primaries = primaries;
    }

    typedef Image* (*CreateFunction)(FILE* f, bool transformOETF,
                                     bool rescaleTo8Bitbool);
    static const std::vector<CreateFunction> CreateFunctions;

    static Image* CreateFromNPBM(FILE*, bool transformOETF = true,
                                 bool rescaleTo8Bitbool = false);
    static Image* CreateFromJPG(FILE* f, bool transformOETF = true,
                                 bool rescaleTo8Bitbool = false);
    static Image* CreateFromPNG(FILE* f, bool transformOETF = true,
                                 bool rescaleTo8Bitbool = false);
    static Image* CreateFromFile(const _tstring& name,
                                 bool transformOETF = true,
                                 bool rescaleTo8Bitbool = false);

    virtual operator uint8_t*() = 0;

    virtual size_t getByteCount() const = 0;
    virtual const uint32_t getPixelSize() const = 0;
    virtual const uint32_t getComponentCount() const = 0;
    virtual const uint32_t getComponentSize() const = 0;
    virtual Image* createImage(uint32_t width, uint32_t height) = 0;
    virtual void resample(Image& dst, bool srgb = false,
                          const char *pFilter = "lanczos4",
                          float filter_scale = 1.0f,
                          basisu::Resampler::Boundary_Op wrapMode
                          = basisu::Resampler::Boundary_Op::BOUNDARY_CLAMP) = 0;
    virtual Image& yflip() = 0;
    virtual Image& transformOETF(OETFFunc decode, OETFFunc encode,
                                 float gamma = 1.0f) = 0;
    virtual Image& swizzle(std::string& swizzle) = 0;
    virtual Image& copyToR(Image&) = 0;
    virtual Image& copyToRG(Image&) = 0;
    virtual Image& copyToRGB(Image&) = 0;
    virtual Image& copyToRGBA(Image&) = 0;

  protected:
    Image() : width(0), height(0), primaries(KHR_DF_PRIMARIES_BT709) { }
    Image(uint32_t w, uint32_t h)
            : width(w), height(h), primaries(KHR_DF_PRIMARIES_BT709) { }

    uint32_t width, height;  // In pixels
    colortype_e colortype;
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
        memset(pixels, 0, bytes);
    }

    ImageT(uint32_t w, uint32_t h, Color* pixels) : Image(w, h), pixels(pixels)
    {
    }

    ~ImageT()
    {
        free(pixels);
    }

    virtual const Color &operator() (uint32_t x, uint32_t y) const {
       assert(x < width && y < height); return pixels[x + y * width];
    }
    virtual Color &operator() (uint32_t x, uint32_t y) {
        assert(x < width && y < height); return pixels[x + y * width];
    }
    virtual operator uint8_t*() { return (uint8_t*)pixels; }

    virtual size_t getByteCount() const {
        return getPixelCount() * sizeof(Color);
    }

    virtual const uint32_t getPixelSize() const {
        return Color::getPixelSize();
    }
    virtual const uint32_t getComponentCount() const {
        return Color::getComponentCount();
    }
    virtual const uint32_t getComponentSize() const {
        return Color::getComponentSize();
    }

    virtual Image* createImage(uint32_t width, uint32_t height) {
        ImageT* image = new ImageT(width, height);
        return image;
    }

    static void checkResamplerStatus(basisu::Resampler& resampler,
                                     const char* pFilter)
    {
        using namespace basisu;

        switch (resampler.status()) {
          case Resampler::Status::STATUS_OKAY:
            break;
          case Resampler::Status::STATUS_OUT_OF_MEMORY:
            throw std::runtime_error("Resampler or Resampler::put_line out of memory.");
            break;
          case Resampler::Status::STATUS_BAD_FILTER_NAME:
          {
            std::string msg("Unknown filter: ");
            msg += pFilter;
            throw std::runtime_error(msg);
            break;
          }
          case Resampler::Status::STATUS_SCAN_BUFFER_FULL:
            throw std::runtime_error("Resampler::put_line scan buffer full.");
            break;
        }
    }

    virtual void resample(Image& abstract_dst, bool srgb, const char *pFilter,
                          float filter_scale,
                          basisu::Resampler::Boundary_Op wrapMode)
    {
        using namespace basisu;

        ImageT& dst = static_cast<ImageT&>(abstract_dst);

        const uint32_t src_w = width, src_h = height;
        const uint32_t dst_w = dst.getWidth(), dst_h = dst.getHeight();
        assert(src_w && src_h && dst_w && dst_h);

        if (::maximum(src_w, src_h) > BASISU_RESAMPLER_MAX_DIMENSION
            || ::maximum(dst_w, dst_h) > BASISU_RESAMPLER_MAX_DIMENSION)
        {
            std::stringstream message;
            message << "Image larger than max supported size of "
                    << BASISU_RESAMPLER_MAX_DIMENSION;
            throw std::runtime_error(message.str());
        }

        // TODO: Consider just using {decode,encode}_sRGB directly.
        float srgb_to_linear_table[256];
        if (srgb) {
          for (int i = 0; i < 256; ++i)
            srgb_to_linear_table[i] = decode_sRGB((float)i * (1.0f/255.0f));
        }

        const int LINEAR_TO_SRGB_TABLE_SIZE = 8192;
        uint8_t linear_to_srgb_table[LINEAR_TO_SRGB_TABLE_SIZE];

        if (srgb)
        {
            for (int i = 0; i < LINEAR_TO_SRGB_TABLE_SIZE; ++i)
              linear_to_srgb_table[i] = (uint8_t)cclamp<int>((int)(255.0f * encode_sRGB((float)i * (1.0f / (LINEAR_TO_SRGB_TABLE_SIZE - 1))) + .5f), 0, 255);
        }

        // Sadly the compiler doesn't realize that getComponentCount() is a
        // constant value for each template so size the arrays to the max.
        // number of components.
        std::vector<float> samples[4];
        Resampler *resamplers[4];

        resamplers[0] = new Resampler(src_w, src_h, dst_w, dst_h,
                                      wrapMode,
                                      0.0f, 1.0f,
                                      pFilter, nullptr, nullptr,
                                      filter_scale, filter_scale,
                                      0, 0);
        checkResamplerStatus(*resamplers[0], pFilter);
        samples[0].resize(src_w);

        for (uint32_t i = 1; i < getComponentCount(); ++i)
        {
            resamplers[i] = new Resampler(src_w, src_h, dst_w, dst_h,
                                          wrapMode,
                                          0.0f, 1.0f,
                                          pFilter,
                                          resamplers[0]->get_clist_x(),
                                          resamplers[0]->get_clist_y(),
                                          filter_scale, filter_scale,
                                          0, 0);
            checkResamplerStatus(*resamplers[i], pFilter);
            samples[i].resize(src_w);
        }

        uint32_t dst_y = 0;

        for (uint32_t src_y = 0; src_y < src_h; ++src_y)
        {
            //const Color *pSrc = &(this(0, src_y));
            Color* pSrc = &((*this)(0, src_y));

            // Put source lines into resampler(s)
            for (uint32_t x = 0; x < src_w; ++x)
            {
                for (uint32_t ci = 0; ci < getComponentCount(); ++ci)
                {
                  const uint32_t v = (*pSrc)[ci];

                  if (!srgb || (ci == 3))
                      samples[ci][x] = v * (1.0f / 255.0f);
                  else
                      samples[ci][x] = srgb_to_linear_table[v];
                }

                pSrc++;
            }

          for (uint32_t ci = 0; ci < getComponentCount(); ++ci)
          {
              if (!resamplers[ci]->put_line(&samples[ci][0]))
              {
                  checkResamplerStatus(*resamplers[ci], pFilter);
              }
          }

          // Now retrieve any output lines
          for (;;)
          {
            uint32_t ci;
            for (ci = 0; ci < getComponentCount(); ++ci)
            {
                const float *pOutput_samples = resamplers[ci]->get_line();
                if (!pOutput_samples)
                    break;

                const bool linear_flag = !srgb || (ci == 3);

                Color* pDst = &dst(0, dst_y);

                for (uint32_t x = 0; x < dst_w; x++)
                {
                    // TODO: Add dithering
                    if (linear_flag) {
                        int j = (int)(255.0f * pOutput_samples[x] + .5f);
                        pDst->set(ci, (componentType)cclamp<int>(j, 0, Color::one()));
                    } else {
                        int j = (int)((LINEAR_TO_SRGB_TABLE_SIZE - 1) * pOutput_samples[x] + .5f);
                        pDst->set(ci, (componentType)linear_to_srgb_table[cclamp<int>(j, 0, LINEAR_TO_SRGB_TABLE_SIZE - 1)]);
                    }

                    pDst++;
                  }
              }
              if (ci < getComponentCount())
                  break;

              ++dst_y;
          }
      }

      for (uint32_t i = 0; i < getComponentCount(); ++i)
          delete resamplers[i];
    }

    virtual ImageT& yflip() {
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

    virtual ImageT& transformOETF(OETFFunc decode, OETFFunc encode,
                                  float gamma = 1.0f) {
        uint32_t pixelCount = getPixelCount();
        for (uint32_t i = 0; i < pixelCount; ++i) {
            Color& c = pixels[i];
            // Don't transform the alpha component. --------  v
            for (uint32_t comp = 0; comp < getComponentCount() && comp < 3; comp++) {
                float brightness = (float)(c[comp]) / Color::one();
                // gamma is only used by decode_gamma. Currently there is no
                // encode_gamma.
                float intensity = decode(brightness, gamma);
                brightness = cclamp(encode(intensity, gamma), 0.0f, 1.0f);
                c.set(comp, roundf(brightness * Color::one()));
            }
        }
        return *this;
    }

    virtual ImageT& swizzle(std::string& swizzle) {
        assert(swizzle.size() == 4);
        for (size_t i = 0; i < getPixelCount(); i++) {
            Color srcPixel = pixels[i];
            for (uint32_t c = 0; c < getComponentCount(); c++) {
                switch (swizzle[c]) {
                  case 'r':
                    pixels[i].set(c, srcPixel[0]);
                    break;
                  case 'g':
                    pixels[i].set(c, srcPixel[1]);
                    break;
                  case 'b':
                    pixels[i].set(c, srcPixel[2]);
                    break;
                  case 'a':
                    pixels[i].set(c, srcPixel[3]);
                    break;
                  case '0':
                    pixels[i].set(c, (componentType)0x00);
                    break;
                  case '1':
                    pixels[i].set(c, (componentType)Color::one());
                    break;
                  default:
                    assert(false);
                }
            }
        }
        return *this;
    }

    template<class DstImage>
    ImageT& copyTo(DstImage& dst) {
        assert(getComponentSize() == dst.getComponentSize());
        assert(width == dst.getWidth() && height == dst.getHeight());

        dst.setOetf(oetf);
        dst.setPrimaries(primaries);
        dst.setColortype((colortype_e)(dst.getComponentCount() + Image::eLuminanceAlpha));
        for (size_t i = 0; i < getPixelCount(); i++) {
            uint32_t c;
            for (c = 0; c < dst.getComponentCount(); c++) {
                if (c < getComponentCount())
                    dst.pixels[i].set(c, pixels[i][c]);
                else
                    break;
            }
            for (; c < dst.getComponentCount(); c++)
                if (c < 3)
                    dst.pixels[i].set(c, (componentType)0);
                else
                    dst.pixels[i].set(c, (componentType)Color::one());
        }
        return *this;
    }

    virtual ImageT& copyToR(Image& dst) { return copyTo((ImageT<componentType, 1>&)dst); }
    virtual ImageT& copyToRG(Image& dst) { return copyTo((ImageT<componentType, 2>&)dst); }
    virtual ImageT& copyToRGB(Image& dst){ return copyTo((ImageT<componentType, 3>&)dst); }
    virtual ImageT& copyToRGBA(Image& dst) { return copyTo((ImageT<componentType, 4>&)dst); }

  protected:
    Color* pixels;
};

using r8color = color<uint8_t, 1>;
using rg8color = color<uint8_t, 2>;
using rgb8color = color<uint8_t, 3>;
using rgba8color = color<uint8_t, 4>;
using r16color = color<uint16_t, 1>;
using rg16color = color<uint16_t, 2>;
using rgb16color = color<uint16_t, 3>;
using rgba16color = color<uint16_t, 4>;

class r8image : public ImageT<uint8_t, 1> {
  public:
    using MyImageT = ImageT<uint8_t, 1>;
    r8image(uint32_t w, uint32_t h) : MyImageT(w, h) { }
    r8image(uint32_t w, uint32_t h, r8color* data)
        : MyImageT(w, h, data) { }
};
class rg8image : public ImageT<uint8_t, 2> {
  public:
    using MyImageT = ImageT<uint8_t, 2>;
    rg8image(uint32_t w, uint32_t h) : MyImageT(w, h) { }
    rg8image(uint32_t w, uint32_t h, rg8color* data)
        : MyImageT(w, h, data) { }
};
class rgb8image : public ImageT<uint8_t, 3> {
  public:
    using MyImageT = ImageT<uint8_t, 3>;
    rgb8image(uint32_t w, uint32_t h) : MyImageT(w, h) { }
    rgb8image(uint32_t w, uint32_t h, rgb8color* data)
        : MyImageT(w, h, data) { }
};
class rgba8image : public ImageT<uint8_t, 4> {
  public:
    using MyImageT = ImageT<uint8_t, 4>;
    rgba8image(uint32_t w, uint32_t h) : MyImageT(w, h) { }
    rgba8image(uint32_t w, uint32_t h, rgba8color* data)
        : MyImageT(w, h, data) { }
};

class r16image : public ImageT<uint16_t, 1> {
  public:
    using MyImageT = ImageT<uint16_t, 1>;
    r16image(uint32_t w, uint32_t h) : MyImageT(w, h) { }
    r16image(uint32_t w, uint32_t h, r16color* data)
        : MyImageT(w, h, data) { }
};
class rg16image : public ImageT<uint16_t, 2> {
  public:
    using MyImageT = ImageT<uint16_t, 2>;
    rg16image(uint32_t w, uint32_t h) : ImageT(w, h) { }
    rg16image(uint32_t w, uint32_t h, rg16color* data)
        : MyImageT(w, h, data) { }
};
class rgb16image : public ImageT<uint16_t, 3> {
  public:
    using MyImageT = ImageT<uint16_t, 3>;
    rgb16image(uint32_t w, uint32_t h) : MyImageT(w, h) { }
    rgb16image(uint32_t w, uint32_t h, rgb16color* data)
        : MyImageT(w, h, data) { }
};
class rgba16image : public ImageT<uint16_t, 4> {
  public:
    using MyImageT = ImageT<uint16_t, 4>;
    rgba16image(uint32_t w, uint32_t h) : MyImageT(w, h) { }
    rgba16image(uint32_t w, uint32_t h, rgba16color* data)
        : MyImageT(w, h, data) { }
};

#endif /* IMAGE_HPP */



