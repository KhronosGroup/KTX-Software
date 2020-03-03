// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

//
// ©2010 The Khronos Group, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

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

#include "argparser.h"
#include "basisu_resampler.h"
#include "basisu_resampler_filters.h"

typedef float (*OETFFunc)(float const);

template <typename T> inline T clamp(T value, T low, T high) {
    return (value < low) ? low : ((value > high) ? high : value);
}
template <typename T> inline T saturate(T value) {
    return clamp<T>(value, 0, 1.0f);
}

template <typename S> inline S maximum(S a, S b) { return (a > b) ? a : b; }
template <typename S> inline S minimum(S a, S b) { return (a < b) ? a : b; }

#if defined(_MSC_VER)
#define INLINE __inline
#else
#define INLINE __inline__
#endif

static INLINE float
encode709(float const intensity) {
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
decode_bt709(float const brightness)
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

static INLINE float
encode_sRGB(float const intensity)
{
    float brightness;
    if (intensity < 0.0031308f)
        brightness = 12.92f * intensity;
    else
        brightness = 1.055f * pow(intensity, 1.0f/2.4f) - 0.055f;

    return brightness;
}

static INLINE float
decode_sRGB(float const brightness)
{
    float intensity;

    if (brightness < .04045f)
        intensity = saturate(brightness * (1.0f/12.92f));
    else
        intensity = saturate(powf((brightness + .055f) * (1.0f/1.055f), 2.4f));

    return intensity;
}

static INLINE float
encode_linear(float const intensity)
{
    return intensity;
}

template <typename T>
class PreAllocator
{
    private:
        T* memory_ptr;
        std::size_t memory_size;

    public:
        typedef std::size_t     size_type;
        typedef T*              pointer;
        typedef const T*        const_pointer;
        typedef T               value_type;

        PreAllocator(T* memory_ptr, std::size_t memory_size) : memory_ptr(memory_ptr), memory_size(memory_size) {}

        PreAllocator(const PreAllocator& other) throw() : memory_ptr(other.address()), memory_size(other.max_size()) {};

        template<typename U>
        PreAllocator(const PreAllocator<U>& other) throw() : memory_ptr(reinterpret_cast<const PreAllocator&>(other).address()), memory_size(other.max_size()) {};

        template<typename U>
        PreAllocator& operator = (const PreAllocator<U>& other) { return *this; }
        PreAllocator<T>& operator = (const PreAllocator& other) { return *this; }
        ~PreAllocator() {}

		pointer address() const { return memory_ptr; }
        // TODO: Figure out what these should really do.
        pointer address(T& x) { return memory_ptr; }
        const pointer address(const T& x) { memory_ptr; }
        pointer allocate(size_type n, const void* hint = 0) {return memory_ptr;}
        void deallocate(T* ptr, size_type n) {}

        size_type max_size() const {return memory_size;}
};

template <typename componentType, uint32_t componentCount>
class color_base {
    public:
       uint32_t const getComponentCount() const { return componentCount; }
       uint32_t const getComponentSize() const {
           return sizeof(componentType);
       }
       uint32_t const getPixelSize() const {
          return componentCount * sizeof(componentType);
       }
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

    enum class eOETF {
      Linear = 0,
      sRGB = 1,
      bt709 = 2,
      Unset = 3
    };

    virtual ~Image() { };

    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    uint32_t getPixelCount() const { return width * height; }
    eOETF getOetf() const { return oetf; }
    void setOetf(eOETF oetf) { this->oetf = oetf; }

    typedef Image* (*CreateFunction)(FILE* f, bool oetfTransform);
    static const std::vector<CreateFunction> CreateFunctions;

    static Image* CreateFromNPBM(FILE*, bool transformOETF = true);
    static Image* CreateFromPNG(FILE* f, bool transformOETF = true);
    static Image* CreateFromFile(_tstring& name, bool transformOETF = true);

    virtual operator uint8_t*() = 0;

    virtual size_t getByteCount() const = 0;
    virtual const uint32_t getPixelSize() const = 0;
    virtual const uint32_t getComponentCount() const = 0;
    virtual const uint32_t getComponentSize() const = 0;
    virtual Image* createImage(uint32_t width, uint32_t height) = 0;
    virtual Image& clear() = 0;
    virtual Image& crop(uint32_t w, uint32_t h) = 0;
    virtual void resample(Image& dst, bool srgb = false,
                          const char *pFilter = "lanczos4",
                          float filter_scale = 1.0f, bool wrapping = false) = 0;
    virtual Image& resize(uint32_t w, uint32_t h) = 0;
    virtual Image& yflip() = 0;
    virtual Image& transformOETF(OETFFunc decode, OETFFunc encode) = 0;

  protected:
    Image() : width(0), height(0) { }
    Image(uint32_t w, uint32_t h) : width(w), height(h) { }

    uint32_t width, height;  // In pixels
    eOETF oetf;

};

// Base class for template and specializations
template<class Color, class Alloc = std::allocator<Color>>
class imageTBase : public Image {
  public:
    using colorVector = std::vector<Color, Alloc>;

    imageTBase(uint32_t w, uint32_t h) { resize(w, h); }
    virtual ~imageTBase() { }

    virtual const Color &operator() (uint32_t x, uint32_t y) const {
       assert(x < width && y < height); return pixels[x + y * width];
    }
    virtual Color &operator() (uint32_t x, uint32_t y) {
        assert(x < width && y < height); return pixels[x + y * width];
    }
    virtual operator uint8_t*() { return (uint8_t*)pixels.data(); }

    virtual size_t getByteCount() const {
        return pixels.size() * sizeof(Color);
    }

    virtual const uint32_t getPixelSize() const {
        return pixels[0].getPixelSize();
    }
    virtual const uint32_t getComponentCount() const {
        return pixels[0].getComponentCount();
    }
    virtual const uint32_t getComponentSize() const {
        return pixels[0].getComponentSize();
    }

    virtual imageTBase& clear() {
        width = 0;
        height = 0;
        pixels.erase(pixels.begin(), pixels.end());
        return *this;
    }

    virtual Image* createImage(uint32_t width, uint32_t height) {
        imageTBase<Color>* image = new imageTBase<Color>(width, height);
        return image;
    }

    virtual imageTBase& crop(uint32_t w, uint32_t h) {
        if (w == width && h == height)
            return *this;

        if (!w || !h) {
            clear();
            return *this;
        }

        pixels.resize(w * h);
        width = w;
        height = h;
        return *this;
    }

    virtual void resample(Image& abstract_dst, bool srgb, const char *pFilter,
                          float filter_scale, bool wrapping)
    {
        using namespace basisu;

        imageTBase<Color>& dst = static_cast<imageTBase<Color>&>(abstract_dst);

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
              linear_to_srgb_table[i] = (uint8_t)::clamp<int>((int)(255.0f * encode_sRGB((float)i * (1.0f / (LINEAR_TO_SRGB_TABLE_SIZE - 1))) + .5f), 0, 255);
        }

        // Sadly the compiler doesn't realize that getComponentCount() is a
        // constant value for each template so size the arrays to the max.
        // number of components.
        std::vector<float> samples[4];
        Resampler *resamplers[4];

        resamplers[0] = new basisu::Resampler(src_w, src_h, dst_w, dst_h,
            wrapping ? Resampler::BOUNDARY_WRAP : Resampler::BOUNDARY_CLAMP,
            0.0f, 1.0f,
            pFilter, nullptr, nullptr, filter_scale, filter_scale, 0, 0);
        samples[0].resize(src_w);

        for (uint32_t i = 1; i < getComponentCount(); ++i)
        {
            resamplers[i] = new Resampler(src_w, src_h, dst_w, dst_h,
                wrapping ? Resampler::BOUNDARY_WRAP : Resampler::BOUNDARY_CLAMP,
                0.0f, 1.0f,
                pFilter, resamplers[0]->get_clist_x(),
                resamplers[0]->get_clist_y(),
                filter_scale, filter_scale, 0, 0);
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
                for (uint32_t i = 0; i < getComponentCount(); i++)
                    delete resamplers[i];
                throw std::runtime_error("Resampler::put_line failed.");
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
                      pDst->set(ci, (uint8_t)::clamp<int>(j, 0, 255));
                    } else {
                      int j = (int)((LINEAR_TO_SRGB_TABLE_SIZE - 1) * pOutput_samples[x] + .5f);
                      pDst->set(ci, linear_to_srgb_table[::clamp<int>(j, 0, LINEAR_TO_SRGB_TABLE_SIZE - 1)]);
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

    virtual imageTBase& resize(uint32_t w, uint32_t h) {
        return crop(w, h);
    }

    virtual imageTBase& yflip() {
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

    virtual imageTBase& transformOETF(OETFFunc decode, OETFFunc encode) {
      typename colorVector::iterator it = pixels.begin();
        for ( ; it < pixels.end(); it++) {
            // Don't transform the alpha component. --------  v
            for (uint32_t comp = 0; comp < it->getComponentCount() && comp < 3; comp++) {
                float brightness = (float)((*it)[comp]) / 255;
                float intensity = decode(brightness);
                brightness = clamp(encode(intensity), 0.0f, 1.0f);
                it->set(comp, roundf(brightness * 255));
            }
        }
        return *this;
    }

  protected:
    imageTBase() : Image() { }
    imageTBase(Alloc alloc) : Image(), pixels(alloc) { }
    imageTBase(Alloc alloc, uint32_t w, uint32_t h) : pixels(alloc) {
        resize(w, h);
    }

    colorVector pixels;
};

template<class Color, class Alloc = std::allocator<Color>>
class ImageT : public imageTBase<Color, Alloc> {
  public:
    using imageTBase<Color, Alloc>::resize;
    ImageT() : imageTBase<Color, Alloc>() { }
    ImageT(uint32_t w, uint32_t h) : imageTBase<Color, Alloc>() {
        resize(w, h);
    }
};

template<class Color>
class ImageT<Color, PreAllocator<Color>> : public imageTBase<Color, PreAllocator<Color>> {
  public:
    using imageTBase<Color, PreAllocator<Color>>::pixels;
    using Image::width;
    using Image::height;
    using Alloc = PreAllocator<Color>;
    ImageT(uint32_t w, uint32_t h, Color* data, size_t pixelCount)
        : imageTBase<Color, Alloc>(Alloc(data, pixelCount)), data(data) {
            // This and PreAllocator is a hack to get "data" to appear in the
            // vector. resize() would overwrite the data.
            width = w;
            height = h;
            pixels.reserve(pixelCount);
    }

    ~ImageT() {
        pixels.erase(pixels.begin(), pixels.end());
        free(data);
    }

    virtual size_t getByteCount() const {
        // Since space has only been reserved, size() will return 0.
        return pixels.max_size() * sizeof(Color);
    }

  private:
    Color* data;
};


class r8image : public ImageT<color<uint8_t, 1>> {
  public:
    r8image(uint32_t w, uint32_t h) : ImageT<color<uint8_t, 1>>(w, h) { }
};
class rg8image : public ImageT<color<uint8_t, 2>> {
  public:
    rg8image(uint32_t w, uint32_t h) : ImageT<color<uint8_t, 2>>(w, h) { }
};
class rgb8image : public ImageT<color<uint8_t, 3>> {
  public:
    rgb8image(uint32_t w, uint32_t h) : ImageT<color<uint8_t, 3>>(w, h) { }
};
class rgba8image : public ImageT<color<uint8_t, 4>> {
  public:
    rgba8image(uint32_t w, uint32_t h) : ImageT<color<uint8_t, 4>>(w, h) { }
};

class r16image : public ImageT<color<uint16_t, 1>> {
  public:
    r16image(uint32_t w, uint32_t h) : ImageT<color<uint16_t, 1>>(w, h) { }
};
class rg16image : public ImageT<color<uint16_t, 2>> {
  public:
    rg16image(uint32_t w, uint32_t h) : ImageT<color<uint16_t, 2>>(w, h) { }
};
class rgb16image : public ImageT<color<uint16_t, 3>> {
  public:
    rgb16image(uint32_t w, uint32_t h) : ImageT<color<uint16_t, 3>>(w, h) { }
};
class rgba16image : public ImageT<color<uint16_t, 4>> {
  public:
    rgba16image(uint32_t w, uint32_t h) : ImageT<color<uint16_t, 4>>(w, h) { }
};

#endif /* IMAGE_HPP */



