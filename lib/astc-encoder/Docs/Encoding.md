# Effective ASTC Encoding

Most texture compression schemes encode a single color format at single
bitrate, so there are relatively few configuration options available to content
creators beyond selecting which compressed format to use.

ASTC on the other hand is an extremely flexible container format which can
compress multiple color formats at multiple bit rates. Inevitably this
flexibility gives rise to questions about how to best use ASTC to encode a
specific color format, or what the equivalent settings are to get a close
match to another compression format.

This page aims to give some guidelines, but note that they are only guidelines
and are not exhaustive so please deviate from them as needed.

## Traditional format reference

The most commonly used non-ASTC compressed formats, their color format, and
their compressed bitrate are shown in the table below.

| Name     | Color Format | Bits/Pixel | Notes            |
| -------- | ------------ | ---------- | ---------------- |
| BC1      | RGB+A        | 4          | RGB565 + 1-bit A |
| BC3      | RGB+A        | 8          | BC1 RGB + BC4 A  |
| BC3nm    | G+R          | 8          | BC1 G   + BC4 R  |
| BC4      | R            | 4          | L8               |
| BC5      | R+G          | 8          | BC1 R + BC1 G    |
| BC6      | RGB (HDR)    | 8          |                  |
| BC7      | RGB / RGBA   | 8          |                  |
| EAC_R11  | R            | 4          | R11              |
| EAC_RG11 | RG           | 8          | RG11             |
| ETC1     | RGB          | 4          | RGB565           |
| ETC2     | RGB+A        | 4          | RGB565 + 1-bit A |
| ETC2+EAC | RGB+A        | 8          | RGB565 + EAC A   |
| PVRTC    | RGBA         | 2 or 4     |                  |

**Note:** BC2 (RGB+A) is not included in the table because it's rarely used in
practice due to poor quality alpha encoding; BC3 is nearly always used instead.

**Note:** Color representations shown with a `+` symbol indicate non-correlated
compression groups; e.g. an `RGB + A` format compresses `RGB` and `A`
independently and does not assume the two signals are correlated. This can be
a strength (it improves quality when compressing non-correlated signals), but
also a weakness (it reduces quality when compressing correlated signals).

# ASTC Format Mapping

The main question which arises with the mapping of another format on to ASTC
is how to handle cases where the input isn't a 4 channel RGBA input. ASTC is a
container format which always decompresses in to a 4 channel RGBA result.
However, the internal compressed representation is very flexible and can store
1-4 channels as needed on a per-block basis.

To get the best quality for a given bitrate, or the lowest bitrate for a given
quality, it is important that as few channels as possible are stored in the
internal representation to avoid wasting coding space.

Specific optimizations in the ASTC coding scheme exist for:

* Encoding the RGB channels as a single luminance channel, so only a single
  value needs to be stored in the coding instead of three.
* Encoding the A channel as a constant 1.0 value, so the coding doesn't
  actually need to store a per-pixel alpha value at all.

... so mapping your inputs given to the compressor to hit these paths is
really important if you want to get the best output quality for your chosen
bitrate.

## Encoding 1-4 channel data

The table below shows the recommended channel usage for data with different
numbers of color channels present in the data.

The coding swizzle should be applied when compressing an image. This can be
handled by the compressor when reading an uncompressed input image by
specifying the swizzle using the `-esw` command line option.

The sampling swizzle is what your should use in your shader programs to read
the data from the compressed texture, assuming no additional API-level channel
swizzling is specified by the application.

| Input Channels |  ASTC Endpoint | Coding Swizzle | Sampling Swizzle   |
| -------------- |  ------------- | -------------- | ------------------ |
| 1              |  L + 1         | `rrr1`         | `.g` <sup>1</sup>  |
| 2              |  L + A         | `rrrg`         | `.ga` <sup>1</sup> |
| 3              |  RGB + 1       | `rgb1`         | `.rgb`             |
| 4              |  RGB + A       | `rgba`         | `.rgba`            |

**1:** Sampling from `g` is preferred to sampling from `r` because it allows a
single shader to be compatible with ASTC, BC1, or ETC formats. BC1 and ETC1
store color endpoints as RGB565 data, so the `g` channel will have higher
precision. For ASTC it doesn't actually make any difference; the same single
channel luminance will be returned for all three of the `.rgb` channels.

## Equivalence with other formats

Based on these channel encoding requirements we can now derive the the ASTC
coding equivalents for most of the other texture compression formats in common
use today.

| Formant  | ASTC Coding Swizzle | ASTC Sampling Swizzle | Notes            |
| -------- | ------------------- | --------------------- | ---------------- |
| BC1      | `rgba` <sup>1</sup> | `.rgba`               |                  |
| BC3      | `rgba`              | `.rgba`               |                  |
| BC3nm    | `gggr`              | `.ag`                 |                  |
| BC4      | `rrr1`              | `.r`                  |                  |
| BC5      | `rrrg`              | `.ra` <sup>2</sup>    |                  |
| BC6      | `rgb1`              | `.rgb`                | HDR profile only |
| BC7      | `rgba`              | `.rgba`               |                  |
| EAC_R11  | `rrr1`              | `.r`                  |                  |
| EAC_RG11 | `rrrg`              | `.ra` <sup>2</sup>    |                  |
| ETC1     | `rgb1`              | `.rgb`                |                  |
| ETC2     | `rgba` <sup>1</sup> | `.rgba`               |                  |
| ETC2+EAC | `rgba`              | `.rgba`               |                  |
| ETC2+EAC | `rgba`              | `.rgba`               |                  |

**1:** ASTC has no equivalent of the 1-bit punch-through alpha encoding
supported by BC1 or ETC2; if alpha is present it will be a full alpha channel.

**2:** ASTC relies on using the L+A color endpoint type for coding efficiency
for two channel data. It therefore has no direct equivalent of a two-plane
format sampled though the `.rg` channels such as BC5 or EAC_RG11. This can
be emulated by setting texture channel swizzles in the runtime API - e.g. via
`glTexParameteri()` for OpenGL ES - although it has been noted that API
controlled swizzles are not available in WebGL.

# Other Considerations

This section outlines some of the other things to consider when encoding
textures using ASTC.

## Encoding non-correlated channels

Most other texture compression formats have a static channel assignment in
terms of the expected data correlation. For example, ETC2+EAC assumes that RGB
are always correlated and that alpha is non-correlated. ASTC can automatically
encode data as either fully correlated across all 4 channels, or with any one
channel assigned to a separate non-correlated partition to the other three.

The non-correlated channel can be changed on a block-by-block basis, so the
compressor can dynamically adjust the coding based on the data present in the
image. This means that there is no need for non-correlated data to be stored
in a specific channel in the input image.

It is however worth noting that the alpha channel is treated differently to
the RGB color channels in some circumstances:

* When coding for sRGB the alpha channel will always be stored in linear space.
* When coding for HDR the alpha channel can optionally be kept as LDR data.

## Encoding normal maps

The best way to store normal maps using ASTC is similar to the scheme used by
BC5; store the X and Y components of a unit-length normal. The Z component of
the normal can be reconstructed in shader code based on the knowledge that the
vector is unit length.

To encode this we therefore want to store two input channels and should
therefore use the `rrrg` coding swizzle, and the `.ga` sampling swizzle. The
OpenGL ES shader code for reconstruction of the Z value is:

    vec3 nml;
    nml.xy = texture(...).ga;                // Load normals (range 0 to 1)
    nml.xy = nml.xy * 2.0 - 1.0;             // Unpack normals (range -1 to +1)
    nml.z = sqrt(1 - dot(nml.xy, nml.xy));   // Compute Z, given unit length

In addition to this it is useful to optimize for angular error in the resulting
vector rather than for absolute color error in the data, which improves the
perceptual quality of the image.

Both the encoding swizzle and the angular error function are enabled by using
the `-normal` command line option.

## Encoding sRGB data

The ASTC LDR profile can compress sRGB encoded color, which is a more
efficient use of bits than storing linear encoded color because the gamma
corrected value distribution more closely matches human perception of
luminance.

For color data it is nearly always a perceptual quality win to use sRGB input
source textures that are then compressed using the ASTC sRGB compression mode
(compress using the `-cs` command line option rather than the `-cl` command
line option). Note that sRGB gamma correction is only applied to the RGB
channels during decode; the alpha channel is always treated as linear encoded
data.

*Important:* The uncompressed input texture provided on the command line must
be stored in the sRGB color space for `-cs` to function correctly.

## Encoding HDR data

HDR data can be encoded just like LDR data, but with some caveats around
handling the alpha channel.

For many use cases the alpha channel is an actual alpha opacity channel and is
therefore used for storing an LDR value between 0 and 1. For these cases use
the `-ch` compressor option which will treat the RGB channels as HDR, but the
A channel as LDR.

For other use cases the alpha channel is simply a fourth data channel which is
also storing an HDR value. For these cases use the `-cH` compressor option
which will treat all channels as HDR data.

- - -

_Copyright © 2019-2022, Arm Limited and contributors. All rights reserved._
