/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2018 Mark Callow.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @internal
 * @file wthelper.h
 * @~English
 *
 * @brief Helper class for writer tests.
 *
 * @author Mark Callow, Edgewise Consulting
 */

#include "gl_format.h"
#include "vkformat_enum.h"
#include "vk_format.h"
#include "ktx.h"
extern "C" {
  #include "ktxint.h"
  #include "texture2.h"
}

struct wthImageInfo {
    GLsizei size;   // Size of the image data in bytes.
    GLubyte* data;  // Pointer to the image data.
};

class wthTexInfo : public ktxTextureCreateInfo {
      public:
        ktx_uint32_t glTypeSize;
        ktx_uint32_t glType;
        ktx_uint32_t glFormat;
        ktx_uint32_t glBaseInternalformat;
        ktx_uint32_t headerPixelHeight;
        ktx_uint32_t headerPixelDepth;
        ktx_uint32_t headerNumLayers;
};

extern "C" KTX_error_code appendLibId(ktxHashList* head,
                                      ktxHashListEntry* writerEntry);

/**
 * @internal @~English
 * @brief Template class for creating writer test helpers.
 *
 * @tparam component_type  the primitive type of a color component.
 * @tparam numComponents   the number of components in a color.
 * @tparam internalformat  the OpenGL internal format enum for the color.
 */
template<typename component_type,
         ktx_uint32_t numComponents, GLenum internalformat>
class WriterTestHelper {
  public:
    enum createFlagBits {
        eNone = 0x00,
        eMipmapped = 0x01,
        eGenerateMipmaps = 0x02,
        eArray = 0x04
    };
    typedef ktx_uint32_t createFlags;

    WriterTestHelper() : writer_ktx2("WriteTestHelper 1.0 __default__") { }

    ~WriterTestHelper() {
        ktxHashList_Destruct(&kvHash);
        ktxHashList_Destruct(&kvHash_ktx2);
    }

    void resize(createFlags flags,
                ktx_uint32_t numLayers, ktx_uint32_t numFaces,
                ktx_uint32_t numDimensions,
                ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth)
    {
        assert(numFaces == 1 || depth == 1);

        this->width = width;
        this->height = height;
        this->depth = depth;
        numLevels = flags & eMipmapped? levelsFromSize(width, height, depth): 1;
        this->numLayers = numLayers;
        this->numFaces=numFaces;
        isArray = flags & eArray ? true : false;
        texinfo.resize(numLevels, numLayers, numFaces, numDimensions,
                       isArray, width, height, depth);

        // Create the image set.
        imageDataSize = 0;
        images.resize(numLevels);
        imageList.resize(numLevels * numLayers * numFaces * depth);
        for (ktx_uint32_t level = 0, count = 0; level < numLevels; level++) {
            ktx_uint32_t levelWidth = MAX(1, width >> level);
            ktx_uint32_t levelHeight = MAX(1, height >> level);
            ktx_uint32_t levelDepth = MAX(1, depth >> level);
            images[level].resize(numLayers);
            for (ktx_uint32_t layer = 0; layer < numLayers; layer++) {
                ktx_uint32_t numImages = numFaces == 6 ? numFaces : levelDepth;
                images[level][layer].resize(numImages);
                for (ktx_uint32_t faceSlice = 0; faceSlice < numImages; faceSlice++) {
                    ktx_uint32_t componentCount, pixelCount;
                    pixelCount = levelWidth * levelHeight;
                    componentCount = pixelCount * numComponents;
                    images[level][layer][faceSlice].resize(componentCount);
                    // Using std::vector avoids warnings in the following
                    // switch due to access past the end of the array, if we
                    // were using an array and numComponents < 4.
                    std::vector<component_type> color;
                    color.resize(numComponents);
                    switch (numComponents) {
                      case 4:
                        color[3] = (component_type)1;
                      case 3:
                        color[2] = (component_type)faceSlice;
                      case 2:
                        color[1] = (component_type)layer;
                      case 1:
                        color[0] = (component_type)level;
                        break;
                    }
                    for (ktx_uint32_t i = 0; i < pixelCount; i++) {
                        for (ktx_uint32_t j = 0; j < numComponents; j++) {
                            ktx_uint32_t ci = i * numComponents + j;
                            images[level][layer][faceSlice][ci] = color[j];
                        }
                    }
                    imageList[count].size
                         = pixelCount * numComponents * sizeof(component_type);
                    imageDataSize += imageList[count].size;
                    imageList[count++].data
                         = (GLubyte*)images[level][layer][faceSlice].data();
                }
            }
        }

        switch (numDimensions) {
          case 1:
            assert(strlen(KTX_ORIENTATION1_FMT) < sizeof(orientation));
            snprintf(orientation, sizeof(orientation), KTX_ORIENTATION1_FMT,
                     'r');
            break;
          case 2:
            assert(strlen(KTX_ORIENTATION2_FMT) < sizeof(orientation));
            snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT,
                     'r', 'd');
            break;
          case 3:
            assert(strlen(KTX_ORIENTATION3_FMT) < sizeof(orientation));
            snprintf(orientation, sizeof(orientation), KTX_ORIENTATION3_FMT,
                     'r', 'd', 'i');
            break;
        }
        assert(4 <= sizeof(orientation_ktx2));
        orientation_ktx2[0] = 'r';
        orientation_ktx2[1] = 'd';
        orientation_ktx2[2] = 'i';
        orientation_ktx2[3] = 0;
        orientation_ktx2[numDimensions] = 0; // Ensure terminating NULL.

        ktxHashList_Construct(&kvHash);
        ktxHashList_AddKVPair(&kvHash, KTX_ORIENTATION_KEY,
                              (unsigned int)strlen(orientation) + 1,
                              orientation);
        ktxHashList_Serialize(&kvHash, &kvDataLen, &kvData);


        ktxHashList_Construct(&kvHash_ktx2);
        ktxHashList_AddKVPair(&kvHash_ktx2, KTX_WRITER_KEY,
                              (ktx_uint32_t)writer_ktx2.size(),
                              writer_ktx2.data());

        // Get the library to add its Id to the writer key so it will be
        // included in the serialized data.
        ktxHashListEntry* pWriter;
        ktxHashList_FindEntry(&kvHash_ktx2, KTX_WRITER_KEY,
                              &pWriter);
        appendLibId(&kvHash_ktx2, pWriter);

        ktxHashList_Serialize(&kvHash_ktx2, &kvDataLenWriter_ktx2, &kvDataWriter_ktx2);
        ktxHashList_AddKVPair(&kvHash_ktx2, KTX_ORIENTATION_KEY,
                              numDimensions + 1,
                              orientation_ktx2);
        ktxHashList_Sort(&kvHash_ktx2);
        ktxHashList_Serialize(&kvHash_ktx2, &kvDataLenAll_ktx2, &kvDataAll_ktx2);
    }

    // Compare the raw images, which are tightly packed, with potentially
    // row padded images from KTX texture.
    bool compareRawImages(ktx_uint8_t* pData) {
        for (ktx_uint32_t level = 0; level < numLevels; level++) {
            ktx_uint32_t faceLodSize = *(ktx_uint32_t*)pData;
            ktx_uint32_t levelWidth = MAX(1, width >> level);
            ktx_uint32_t levelHeight = MAX(1, height >> level);
            ktx_uint32_t levelDepth = MAX(1, depth >> level);
            ktx_uint32_t numImages;
            ktx_uint32_t rowPadding;
            ktx_size_t paddedImageBytes, imageBytes;
            ktx_size_t paddedRowBytes, rowBytes;
            ktx_size_t expectedFaceLodSize;

            imageBytes = images[level][0][0].size() * sizeof(component_type);
            rowBytes = levelWidth
                            * sizeof(component_type)
                            * numComponents;
            rowPadding = 3 - ((rowBytes + KTX_GL_UNPACK_ALIGNMENT-1) % KTX_GL_UNPACK_ALIGNMENT);
            paddedRowBytes = rowBytes + rowPadding;
            paddedImageBytes = paddedRowBytes * levelHeight;
            if (numFaces == 6 && !isArray) {
                // Non-array cubemap.
                numImages = numFaces;
                expectedFaceLodSize = paddedImageBytes;
            } else {
                numImages = numFaces == 6 ? numFaces : levelDepth;
                expectedFaceLodSize = paddedImageBytes * numImages * numLayers;
            }
            if (faceLodSize != expectedFaceLodSize)
               return false;
            pData += sizeof(ktx_uint32_t);
            for (ktx_uint32_t layer = 0; layer < numLayers; layer++) {
                for (ktx_uint32_t faceSlice = 0; faceSlice < numImages; faceSlice++) {
                    if (rowPadding == 0) {
                        if (memcmp(images[level][layer][faceSlice].data(),
                                   pData,
                                   images[level][layer][faceSlice].size() * sizeof(component_type)))
                            return false;
                        pData += paddedImageBytes;
                    } else {
                        ktx_uint8_t* pImage = (ktx_uint8_t*)images[level][layer][faceSlice].data();
                        for (ktx_uint32_t row = 0; row < levelHeight; row++) {
                            if (memcmp(pImage, pData, rowBytes))
                                return false;
                            pImage += rowBytes;
                            pData += paddedRowBytes;
                        }
                    }
                }
            }
        }
        return true;
    }


    // Compare the raw images, which are tightly packed, with the images from
    // a KTX 2 texture, which are also tightly packed but have reversed order
    // for mip levels.
    bool compareRawImages(ktxLevelIndexEntry levelIndex[], ktx_uint8_t* baseAddr) {
        for (ktx_uint32_t level = 0; level < numLevels; level++) {
            ktx_uint64_t levelSize = levelIndex[level].uncompressedByteLength;
            ktx_uint32_t levelDepth = MAX(1, depth >> level);
            ktx_uint32_t numImages;
            ktx_size_t imageBytes;
            ktx_size_t expectedLevelSize;

            imageBytes = images[level][0][0].size() * sizeof(component_type);
            numImages = numFaces == 6 ? numFaces : levelDepth;
            expectedLevelSize = imageBytes * numImages * numLayers;

            if (levelSize != expectedLevelSize)
               return false;

            ktx_uint8_t* pData = baseAddr + levelIndex[level].byteOffset;
            for (ktx_uint32_t layer = 0; layer < numLayers; layer++) {
                for (ktx_uint32_t faceSlice = 0; faceSlice < numImages; faceSlice++) {
#if 0 //DUMP_IMAGE
                    fprintf(stdout, "Reading level %d, layer %d, faceSlice %d at offset %#" PRIx64 "\n",
                            level, layer, faceSlice, levelIndex[level].offset);
                    for (uint32_t i = 0; i < imageBytes; i++)
                        fprintf(stdout, "%#x, ", *(pData + i));
                    fprintf(stdout, "\n");
#endif

                    if (memcmp(images[level][layer][faceSlice].data(), pData,
                                imageBytes))
                        return false;
                    pData += imageBytes;
                }
            }
        }
        return true;
    }

    static ktx_uint32_t
    levelsFromSize(ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth) {
        ktx_uint32_t mipLevels;
        ktx_uint32_t max_dim = MAX(MAX(width, height), depth);
        for (mipLevels = 1; max_dim != 1; mipLevels++, max_dim >>= 1) { }
        return mipLevels;
    }

    ktx_uint32_t numLevels;
    ktx_uint32_t numLayers;
    ktx_uint32_t numFaces;
    ktx_uint32_t width;
    ktx_uint32_t height;
    ktx_uint32_t depth;
    bool isArray;

    ktx_uint8_t* kvData;
    ktx_uint32_t kvDataLen;
    char orientation[15];

    ktx_uint8_t* kvDataWriter_ktx2;
    ktx_uint32_t kvDataLenWriter_ktx2;
    ktx_uint8_t* kvDataAll_ktx2;
    ktx_uint32_t kvDataLenAll_ktx2;
    ktxHashList kvHash;
    ktxHashList kvHash_ktx2;
    char orientation_ktx2[4];
    std::string writer_ktx2;
    std::string comparisonWriter_ktx2;
    

    ktx_size_t imageDataSize;
    std::vector< std::vector < std::vector < std::vector<component_type> > > > images;
    std::vector<wthImageInfo> imageList;

    class texinfo : public wthTexInfo {
      public:
        texinfo() {
            glType = glGetTypeFromInternalFormat(internalformat);
            glTypeSize = glGetTypeSizeFromType(glType);
            glFormat = glGetFormatFromInternalFormat(internalformat);
            glInternalformat = internalformat;
            glBaseInternalformat = glFormat;
        }

        void resize(GLuint numLevels, GLuint numLayers, GLuint numFaces,
                    GLuint numDimensions, bool isArray,
                    GLsizei width, GLsizei height, GLsizei depth) {
            this->numLayers = numLayers;
            this->numFaces = numFaces;
            this->numLevels = numLevels;
            this->numDimensions = numDimensions;
            this->generateMipmaps = false;
            this->isArray = isArray;
            baseWidth = width;
            baseHeight = height;
            baseDepth = depth;
            headerNumLayers = isArray ? numLayers: 0;
            headerPixelHeight = numDimensions >= 2 ? height : 0;
            headerPixelDepth = numDimensions == 3 ? depth : 0;
       }

        bool compare(KTX_header* header) {
            if (header->glType == glType
                && header->glTypeSize == glTypeSize
                && header->glFormat == glFormat
                && header->glInternalformat == glInternalformat
                && header->glBaseInternalformat == glBaseInternalformat
                && header->pixelWidth == baseWidth
                && header->pixelHeight == headerPixelHeight
                && header->pixelDepth == headerPixelDepth
                && header->numberOfArrayElements == headerNumLayers
                && header->numberOfFaces == numFaces
                && header->numberOfMipLevels == numLevels)
                return true;
            else
                return false;
        }

        bool compare(KTX_header2* header) {
            VkFormat format =
            vkGetFormatFromOpenGLInternalFormat(glInternalformat);

            // Should find better way to test this. Code we're testing uses the
            // same switch to convert format.
            if (header->vkFormat == format
                && header->pixelWidth == baseWidth
                && header->pixelHeight == headerPixelHeight
                && header->pixelDepth == headerPixelDepth
                && header->layerCount == headerNumLayers
                && header->faceCount == numFaces
                && header->levelCount == numLevels
                && header->supercompressionScheme >= KTX_SS_BEGIN_RANGE
                && header->supercompressionScheme <= KTX_SS_END_RANGE)
                return true;
            else
                return false;
        }

        bool compare(ktxTexture2* texture) {
            VkFormat format =
            vkGetFormatFromOpenGLInternalFormat(glInternalformat);

            if (texture->vkFormat == format
                && texture->baseWidth == baseWidth
                && texture->baseHeight == baseHeight
                && texture->baseDepth == baseDepth
                && texture->numLayers == numLayers
                && texture->numFaces == numFaces
                && texture->numLevels == numLevels
                && texture->supercompressionScheme >= KTX_SS_BEGIN_RANGE
                && texture->supercompressionScheme <= KTX_SS_END_RANGE)
                return true;
            else
                return false;
        }
    } texinfo;
};
