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

typedef float (*OETFFunc)(float const);

template <typename T> inline T clamp(T value, T low, T high) {
    return (value < low) ? low : ((value > high) ? high : value);
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

        PreAllocator(const PreAllocator& other) throw() : memory_ptr(other.memory_ptr), memory_size(other.memory_size) {};

        template<typename U>
        PreAllocator(const PreAllocator<U>& other) throw() : memory_ptr(other.memory_ptr), memory_size(other.memory_size) {};

        template<typename U>
        PreAllocator& operator = (const PreAllocator<U>& other) { return *this; }
        PreAllocator<T>& operator = (const PreAllocator& other) { return *this; }
        ~PreAllocator() {}

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
       uint32_t getComponentCount() const { return componentCount; }
       uint32_t getComponentSize() const { return sizeof(componentType); }
       uint32_t getPixelSize() const {
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

    enum oetf_e {
      eOETFLinear = 0,
      eOETFsRGB = 1,
      eOETF709 = 2,
      eOETFUnset = 3
    };

    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    uint32_t getPixelCount() const { return width * height; }
    oetf_e getOetf() const { return oetf; }
    void setOetf(oetf_e oetf) { this->oetf = oetf; }

    typedef Image* (*CreateFunction)(FILE* f, bool oetfTransform);
    static const std::vector<CreateFunction> CreateFunctions;

    static Image* CreateFromNPBM(FILE*, bool transformOETF = true);
    static Image* CreateFromPNG(FILE* f, bool transformOETF = true);
    static Image* CreateFromFile(_tstring& name, bool transformOETF = true);

    virtual operator uint8_t*() = 0;
    virtual size_t getByteCount() = 0;
    virtual uint32_t getPixelSize() = 0;
    virtual uint32_t getComponentCount() = 0;
    virtual uint32_t getComponentSize() = 0;
    virtual Image& clear() = 0;
    virtual Image& crop(uint32_t w, uint32_t h) = 0;
    virtual Image& resize(uint32_t w, uint32_t h) = 0;
    virtual Image& yflip() = 0;
    virtual Image& transformOETF(OETFFunc decode, OETFFunc encode) = 0;

  protected:
    Image() : width(0), height(0) { }
    Image(uint32_t w, uint32_t h) : width(w), height(h) { }

    uint32_t width, height;  // In pixels
    oetf_e oetf;

};

// Base class for template and specializations
template<class Color, class Alloc = std::allocator<Color>>
class imageTBase : public Image {
  public:
    using colorVector = std::vector<Color, Alloc>;

    virtual operator uint8_t*() { return (uint8_t*)pixels.data(); }

    virtual size_t getByteCount() {
        return pixels.size() * sizeof(Color);
    }

    virtual uint32_t getPixelSize() { return pixels[0].getPixelSize(); }
    virtual uint32_t getComponentCount() {
        return pixels[0].getComponentCount();
    }
    virtual uint32_t getComponentSize() { return pixels[0].getComponentSize(); }

    virtual imageTBase& clear() {
        width = 0;
        height = 0;
        pixels.erase(pixels.begin(), pixels.end());
        return *this;
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

    virtual imageTBase& resize(uint32_t w, uint32_t h) {
        return crop(w, h);
    }

    virtual imageTBase& yflip() {
        uint32_t rowSize = width * sizeof(Color);
        // Minimize memory use by only buffering a single row.
        Color* rowBuffer = new Color[width];

        for (int sy = height-1, dy = 0; sy >= height / 2; sy--, dy++) {
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
    imageTBase(uint32_t w, uint32_t h) : Image(w, h) { }
    imageTBase(Alloc alloc, uint32_t w, uint32_t h)
        : Image(w, h), pixels(alloc) { }

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
    using Alloc = PreAllocator<Color>;
    ImageT(uint32_t w, uint32_t h, Color* data, size_t pixelCount)
        : imageTBase<Color, Alloc>(Alloc(data, pixelCount), w, h), data(data) {
            // This and PreAllocator is a hack to get "data" to appear in the
            // vector. resize() would overwrite the data.
            pixels.reserve(pixelCount);
    }

    ImageT() {
        pixels.erase();
        free(data);
    }

    virtual size_t getByteCount() {
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
decode709(float const brightness)
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

#endif /* IMAGE_HPP */



