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
#include "ktx.h"
extern "C" {
  #include "ktxint.h"
}

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

    WriterTestHelper() { }

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
        
        assert(strlen(KTX_ORIENTATION2_FMT) < sizeof(orientation));
        snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT,
                 'r', 'd');
        ktxHashList* hl;
        ktxHashList_Create(&hl);
        ktxHashList_AddKVPair(hl, KTX_ORIENTATION_KEY,
                              (unsigned int)strlen(orientation) + 1,
                              orientation);
        ktxHashList_Serialize(hl, &kvDataLen, &kvData);
        ktxHashList_Destruct(hl);
    }

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
    char orientation[10];

    ktx_size_t imageDataSize;
    std::vector< std::vector < std::vector < std::vector<component_type> > > > images;
    std::vector<KTX_image_info> imageList;

    class texinfo : public KTX_texture_info {
      public:
        texinfo() {
            glType = glGetTypeFromInternalFormat(internalformat);
            glTypeSize = glGetTypeSizeFromType(glType);
            glFormat = glGetFormatFromInternalFormat(internalformat);
            glInternalFormat = internalformat;
            glBaseInternalFormat = glFormat;
        }

        void resize(GLuint numLevels, GLuint numLayers, GLuint numFaces,
                    GLuint numDimensions, bool isArray,
                    GLsizei width, GLsizei height, GLsizei depth) {
            numberOfArrayElements = isArray ? numLayers: 0;
            numberOfFaces = numFaces;
            numberOfMipmapLevels = numLevels;
            pixelWidth = width;
            pixelHeight = numDimensions >= 2 ? height : 0;
            pixelDepth = numDimensions == 3 ? depth : 0;
        }

        bool compare(KTX_header* header) {
            if (header->glType == glType
                && header->glTypeSize == glTypeSize
                && header->glFormat == glFormat
                && header->glInternalformat == glInternalFormat
                && header->glBaseInternalformat == glBaseInternalFormat
                && header->pixelWidth == pixelWidth
                && header->pixelHeight == pixelHeight
                && header->pixelDepth == pixelDepth
                && header->numberOfArrayElements == numberOfArrayElements
                && header->numberOfFaces == numberOfFaces
                && header->numberOfMipmapLevels == numberOfMipmapLevels)
                return true;
            else
                return false;
        }
    } texinfo;
};
