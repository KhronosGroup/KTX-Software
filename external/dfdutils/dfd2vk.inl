/* Copyright 2019-2020 The Khronos Group Inc. */
/* SPDX-License-Identifier: Apache-2.0 */

/***************************** Do not edit.  *****************************
             Automatically generated by makedfd2vk.pl.
 *************************************************************************/
if (KHR_DFDVAL(dfd + 1, MODEL) == KHR_DF_MODEL_RGBSDA || KHR_DFDVAL(dfd + 1, MODEL) == KHR_DF_MODEL_YUVSDA) {
  enum InterpretDFDResult r;
  InterpretedDFDChannel R = {0,0};
  InterpretedDFDChannel G = {0,0};
  InterpretedDFDChannel B = {0,0};
  InterpretedDFDChannel A = {0,0};
  /* interpretDFD channel overloadings for YUVSDA formats. These are
   * different from the mapping used by Vulkan. */
  #define Y1 R
  #define Y2 A
  #define CB G
  #define U G
  #define CR B
  #define V B
  uint32_t wordBytes;

  /* Special case exponent format */
  if (KHR_DFDSAMPLECOUNT(dfd + 1) == 6 &&
      ((KHR_DFDSVAL((dfd + 1), 1, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_EXPONENT) > 0)) {
    /* The only format we expect to be encoded like this. */
    return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
  }

  /* Special case depth formats (assumed little-endian) */
  if (KHR_DFDSVAL((dfd + 1), 0, CHANNELID) == KHR_DF_CHANNEL_RGBSDA_DEPTH) {
    if (KHR_DFDSAMPLECOUNT((dfd + 1)) == 1) {
      if (KHR_DFDSVAL((dfd + 1), 0, BITLENGTH) == 16-1) return VK_FORMAT_D16_UNORM;
      if (KHR_DFDSVAL((dfd + 1), 0, BITLENGTH) == 24-1) return VK_FORMAT_X8_D24_UNORM_PACK32;
      return VK_FORMAT_D32_SFLOAT;
    } else {
      if (KHR_DFDSVAL((dfd + 1), 0, BITLENGTH) == 16-1) return VK_FORMAT_D16_UNORM_S8_UINT;
      if (KHR_DFDSVAL((dfd + 1), 0, BITLENGTH) == 24-1) return VK_FORMAT_D24_UNORM_S8_UINT;
      return VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
  }
  if (KHR_DFDSVAL((dfd + 1), 0, CHANNELID) == KHR_DF_CHANNEL_RGBSDA_STENCIL) {
    if (KHR_DFDSAMPLECOUNT((dfd + 1)) == 1) {
      return VK_FORMAT_S8_UINT;
    } else {
      // The KTX 2.0 specification defines D24_UNORM_S8_UINT with S8 in the LSBs
      return VK_FORMAT_D24_UNORM_S8_UINT;
    }
  }

  r = interpretDFD(dfd, &R, &G, &B, &A, &wordBytes);

  if (r & i_UNSUPPORTED_ERROR_BIT) return VK_FORMAT_UNDEFINED;

  if (r & i_PACKED_FORMAT_BIT) {
    if (wordBytes == 1) return VK_FORMAT_R4G4_UNORM_PACK8;
    else if (wordBytes == 2) { /* PACK16 */
      if (A.size == 4) {
        if (R.offset == 12) return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        else if (B.offset == 12) return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
        else if (A.offset == 12) {
          if (R.offset == 8) return VK_FORMAT_A4R4G4B4_UNORM_PACK16;
          else return VK_FORMAT_A4B4G4R4_UNORM_PACK16;
        }
      } else if (G.size == 0 && B.size == 0 && A.size == 0) { /* One channel */
        if (R.size == 10)
          return VK_FORMAT_R10X6_UNORM_PACK16;
        else if (R.size ==12)
          return VK_FORMAT_R12X4_UNORM_PACK16;
       } else if (A.size == 0) { /* Three channels */
        if (B.offset == 0) return VK_FORMAT_R5G6B5_UNORM_PACK16;
        else return VK_FORMAT_B5G6R5_UNORM_PACK16;
      } else { /* Four channels, one-bit alpha */
        if (B.offset == 0) return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
        if (B.offset == 1) return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
        if (B.offset == 10) return VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR;
        return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
      }
    } else if (wordBytes == 4) { /* PACK32 or 2PACK16 */
      if (A.size == 8) {
        if ((r & i_SRGB_FORMAT_BIT)) return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
        if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
        if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A8B8G8R8_SNORM_PACK32;
        if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A8B8G8R8_UINT_PACK32;
        if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A8B8G8R8_SINT_PACK32;
      } else if (A.size == 2 && B.offset == 0) {
        if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
        if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A2R10G10B10_SINT_PACK32;
      } else if (A.size == 2 && R.offset == 0) {
        if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
        if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_A2B10G10R10_SINT_PACK32;
      } else if (R.size == 11) {
          return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
      } else if (R.size == 10 && G.size == 10 && B.size == 0) { 
          return VK_FORMAT_R10X6G10X6_UNORM_2PACK16;
      } else if (R.size == 12 && G.size == 12 && B.size == 0) { 
          return VK_FORMAT_R12X4G12X4_UNORM_2PACK16;
      }
    } else if (wordBytes == 8) { /* 4PACK16 */
      if (r & i_YUVSDA_FORMAT_BIT) {
        /* In Vulkan G = Y, R = Cr, B = Cb. */
        if (Y1.size == 10 && Y1.offset == 6 && Y2.size == 10 && Y2.offset == 38)
          return  VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
        if (Y1.size == 10 && Y1.offset == 22 && Y2.size == 10 && Y2.offset == 54)
          return VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
        if (Y1.size == 12 && Y1.offset == 4 && Y2.size == 12 && Y2.offset == 36)
          return VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
        if (Y1.size == 12 && Y1.offset == 20 && Y2.size == 12 && Y2.offset == 52)
          return VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
      } else {
        if (R.size == 10)
          return VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;
        else if (R.size == 12)
          return VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;
      }
    }
  } else { /* Not a packed format */
    if (r & i_YUVSDA_FORMAT_BIT) {
      /* In Vulkan G = Y, R = Cr, B = Cb. */
      if (Y1.size == 1 && Y1.offset == 0 && Y2.size == 1 && Y2.offset == 2)
        return VK_FORMAT_G8B8G8R8_422_UNORM;
      else if (Y1.size == 1 && Y1.offset == 1 && Y2.size == 1 && Y2.offset == 3)
        return VK_FORMAT_B8G8R8G8_422_UNORM;
      else if (Y1.size == 2 && Y1.offset == 0 && Y2.size == 2 && Y2.offset == 4)
        return VK_FORMAT_G16B16G16R16_422_UNORM;
      else if (Y1.size == 2 && Y1.offset == 2 && Y2.size == 2 && Y2.offset == 6)
        return VK_FORMAT_B16G16R16G16_422_UNORM;
      else
        return VK_FORMAT_UNDEFINED; // Until support added.
    } else { /* Not YUV */
      if (wordBytes == 1) {
        if (A.size == 1 && R.size == 0 && G.size == 0 && B.size == 0 && (r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) {
            return VK_FORMAT_A8_UNORM_KHR;
        }
        if (A.size > 0) { /* 4 channels */
          if (R.offset == 0) { /* RGBA */
            if ((r & i_SRGB_FORMAT_BIT)) return VK_FORMAT_R8G8B8A8_SRGB;
            if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8B8A8_UNORM;
            if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8B8A8_SNORM;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8B8A8_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8B8A8_SINT;
          } else { /* BGRA */
            if ((r & i_SRGB_FORMAT_BIT)) return VK_FORMAT_B8G8R8A8_SRGB;
            if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_B8G8R8A8_UNORM;
            if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_B8G8R8A8_SNORM;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_B8G8R8A8_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_B8G8R8A8_SINT;
          }
        } else if (B.size > 0) { /* 3 channels */
          if (R.offset == 0) { /* RGB */
            if ((r & i_SRGB_FORMAT_BIT)) return VK_FORMAT_R8G8B8_SRGB;
            if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8B8_UNORM;
            if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8B8_SNORM;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8B8_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8B8_SINT;
          } else { /* BGR */
            if ((r & i_SRGB_FORMAT_BIT)) return VK_FORMAT_B8G8R8_SRGB;
            if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_B8G8R8_UNORM;
            if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_B8G8R8_SNORM;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_B8G8R8_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_B8G8R8_SINT;
          }
        } else if (G.size > 0) { /* 2 channels */
          if ((r & i_SRGB_FORMAT_BIT)) return VK_FORMAT_R8G8_SRGB;
          if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8_UNORM;
          if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8_SNORM;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8_UINT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8G8_SINT;
        } else { /* 1 channel */
          if ((r & i_SRGB_FORMAT_BIT)) return VK_FORMAT_R8_SRGB;
          if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8_UNORM;
          if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8_SNORM;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8_UINT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R8_SINT;
        }
      } else if (wordBytes == 2) {
        if ((r & i_FIXED_FORMAT_BIT) && R.size == 2 && G.size == 2)  return  VK_FORMAT_R16G16_SFIXED5_NV;
        if (A.size > 0) { /* 4 channels */
          if (R.offset == 0) { /* RGBA */
            if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R16G16B16A16_SFLOAT;
            if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16B16A16_UNORM;
            if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16B16A16_SNORM;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16B16A16_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16B16A16_SINT;
          } else { /* BGRA */
          }
        } else if (B.size > 0) { /* 3 channels */
          if (R.offset == 0) { /* RGB */
            if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R16G16B16_SFLOAT;
            if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16B16_UNORM;
            if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16B16_SNORM;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16B16_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16B16_SINT;
          } else { /* BGR */
          }
        } else if (G.size > 0) { /* 2 channels */
          if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R16G16_SFLOAT;
          if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16_UNORM;
          if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16_SNORM;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16_UINT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16G16_SINT;
        } else { /* 1 channel */
          if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R16_SFLOAT;
          if ((r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16_UNORM;
          if ((r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16_SNORM;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16_UINT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R16_SINT;
        }
      } else if (wordBytes == 4) {
        if (A.size > 0) { /* 4 channels */
          if (R.offset == 0) { /* RGBA */
            if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R32G32B32A32_SFLOAT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R32G32B32A32_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R32G32B32A32_SINT;
          } else { /* BGRA */
          }
        } else if (B.size > 0) { /* 3 channels */
          if (R.offset == 0) { /* RGB */
            if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R32G32B32_SFLOAT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R32G32B32_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R32G32B32_SINT;
          } else { /* BGR */
          }
        } else if (G.size > 0) { /* 2 channels */
          if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R32G32_SFLOAT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R32G32_UINT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R32G32_SINT;
        } else { /* 1 channel */
          if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R32_SFLOAT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R32_UINT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R32_SINT;
        }
      } else if (wordBytes == 8) {
        if (A.size > 0) { /* 4 channels */
          if (R.offset == 0) { /* RGBA */
            if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R64G64B64A64_SFLOAT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R64G64B64A64_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R64G64B64A64_SINT;
          } else { /* BGRA */
          }
        } else if (B.size > 0) { /* 3 channels */
          if (R.offset == 0) { /* RGB */
            if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R64G64B64_SFLOAT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R64G64B64_UINT;
            if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R64G64B64_SINT;
          } else { /* BGR */
          }
        } else if (G.size > 0) { /* 2 channels */
          if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R64G64_SFLOAT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R64G64_UINT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R64G64_SINT;
        } else { /* 1 channel */
          if ((r & i_FLOAT_FORMAT_BIT)) return VK_FORMAT_R64_SFLOAT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && !(r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R64_UINT;
          if (!(r & i_NORMALIZED_FORMAT_BIT) && (r & i_SIGNED_FORMAT_BIT)) return VK_FORMAT_R64_SINT;
        }
      }
    }
  }
} else if (KHR_DFDVAL((dfd + 1), MODEL) >= 128) {
  const uint32_t *bdb = dfd + 1;
  switch (KHR_DFDVAL(bdb, MODEL)) {
  case KHR_DF_MODEL_BC1A:
    if (KHR_DFDSVAL(bdb, 0, CHANNELID) == KHR_DF_CHANNEL_BC1A_COLOR) {
      if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
        return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
      } else {
        return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
      }
    } else {
      if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
      } else {
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
      }
    }
  case KHR_DF_MODEL_BC2:
    if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
      return VK_FORMAT_BC2_UNORM_BLOCK;
    } else {
      return VK_FORMAT_BC2_SRGB_BLOCK;
    }
  case KHR_DF_MODEL_BC3:
    if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
      return VK_FORMAT_BC3_UNORM_BLOCK;
    } else {
      return VK_FORMAT_BC3_SRGB_BLOCK;
    }
  case KHR_DF_MODEL_BC4:
    if (!(KHR_DFDSVAL(bdb, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_SIGNED)) {
      return VK_FORMAT_BC4_UNORM_BLOCK;
    } else {
      return VK_FORMAT_BC4_SNORM_BLOCK;
    }
  case KHR_DF_MODEL_BC5:
    if (!(KHR_DFDSVAL(bdb, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_SIGNED)) {
      return VK_FORMAT_BC5_UNORM_BLOCK;
    } else {
      return VK_FORMAT_BC5_SNORM_BLOCK;
    }
  case KHR_DF_MODEL_BC6H:
    if (!(KHR_DFDSVAL(bdb, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_SIGNED)) {
      return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    } else {
      return VK_FORMAT_BC6H_SFLOAT_BLOCK;
    }
  case KHR_DF_MODEL_BC7:
    if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
      return VK_FORMAT_BC7_UNORM_BLOCK;
    } else {
      return VK_FORMAT_BC7_SRGB_BLOCK;
    }
  case KHR_DF_MODEL_ETC2:
    if (KHR_DFDSVAL(bdb, 0, CHANNELID) == KHR_DF_CHANNEL_ETC2_COLOR) {
      if (KHR_DFDVAL(bdb, DESCRIPTORBLOCKSIZE) == 40) {
        if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
          return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        } else {
          return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        }
      } else {
        if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
          return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        } else {
          return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
        }
      }
    } else if (KHR_DFDSVAL(bdb, 0, CHANNELID) == KHR_DF_CHANNEL_ETC2_ALPHA) {
      if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
        return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
      } else {
        return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
      }
    } else if (KHR_DFDVAL(bdb, DESCRIPTORBLOCKSIZE) == 40) {
      if (!(KHR_DFDSVAL(bdb, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_SIGNED)) {
        return VK_FORMAT_EAC_R11_UNORM_BLOCK;
      } else {
        return VK_FORMAT_EAC_R11_SNORM_BLOCK;
      }
    } else {
      if (!(KHR_DFDSVAL(bdb, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_SIGNED)) {
        return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
      } else {
        return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
      }
    }
  case KHR_DF_MODEL_ASTC:
    if (!(KHR_DFDSVAL(bdb, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_FLOAT)) {
      if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 0) {
        if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 7) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 7) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 7) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 7)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 9) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 9) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 9) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 7)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 9) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 9)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 11) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 9)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 11) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 11)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
          } else {
            return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
          }
        }
      } else {
        if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 2) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 2) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 2)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 2) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 2)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 2)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 3)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT;
          }
        }
        if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 3)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 3)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 4)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 4)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 4)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT;
          }
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 5)) {
          if (KHR_DFDVAL(bdb, TRANSFER) != KHR_DF_TRANSFER_SRGB) {
            return VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT;
          } else {
            return VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT;
          }
        }
      }
    } else {
      if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 0) {
        if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3)) {
          return VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3)) {
          return VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4)) {
          return VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4)) {
          return VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5)) {
          return VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 7) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4)) {
          return VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 7) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5)) {
          return VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 7) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 7)) {
          return VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 9) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4)) {
          return VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 9) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5)) {
          return VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 9) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 7)) {
          return VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 9) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 9)) {
          return VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 11) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 9)) {
          return VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 11) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 11)) {
          return VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT;
        }
      } else {
        if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 2) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 2) &&
            (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 2)) {
          return VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 2) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 2)) {
          return VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 2) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 2)) {
          return VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 2)) {
          return VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 3)) {
          return VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 3) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 3)) {
          return VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 3)) {
          return VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 4)) {
          return VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 4) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 4)) {
          return VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 4)) {
          return VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT;
        } else if ((KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) == 5) &&
                   (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) == 5)) {
          return VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT;
        }
      }
    }
    break;
  case KHR_DF_MODEL_PVRTC:
    if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) {
      if (KHR_DFDVAL(bdb, TRANSFER) == KHR_DF_TRANSFER_SRGB) {
        return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
      } else {
        return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
      }
    } else {
      if (KHR_DFDVAL(bdb, TRANSFER) == KHR_DF_TRANSFER_SRGB) {
        return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
      } else {
        return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
      }
    }
  case KHR_DF_MODEL_PVRTC2:
    if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) == 3) {
      if (KHR_DFDVAL(bdb, TRANSFER) == KHR_DF_TRANSFER_SRGB) {
        return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
      } else {
        return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
      }
    } else {
      if (KHR_DFDVAL(bdb, TRANSFER) == KHR_DF_TRANSFER_SRGB) {
        return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;
      } else {
        return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;
      }
    }
  default:
    ;
  }
}
return VK_FORMAT_UNDEFINED; /* Drop-through for unmatched formats. */
