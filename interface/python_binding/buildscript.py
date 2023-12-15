# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from cffi import FFI
import platform
import sys
import os
import unittest

LIBKTX_INSTALL_DIR = os.getenv("LIBKTX_INSTALL_DIR")
LIBKTX_INCLUDE_DIR = os.getenv("LIBKTX_INCLUDE_DIR")
LIBKTX_LIB_DIR = os.getenv("LIBKTX_LIB_DIR")

if os.name == 'nt':
    if LIBKTX_INSTALL_DIR is None:
        LIBKTX_INSTALL_DIR = 'C:\\Program Files\\KTX-Software'
    if LIBKTX_INCLUDE_DIR is None:
        LIBKTX_INCLUDE_DIR = LIBKTX_INSTALL_DIR + '\\include'
    if LIBKTX_LIB_DIR is None:
        LIBKTX_LIB_DIR = LIBKTX_INSTALL_DIR + '\\lib'
elif platform.system() == 'Darwin':
    if LIBKTX_INCLUDE_DIR is None:
        LIBKTX_INCLUDE_DIR = '/usr/local/include'
    if LIBKTX_LIB_DIR is None:
        LIBKTX_LIB_DIR = '/usr/local/lib'
elif os.name == 'posix':
    if LIBKTX_INCLUDE_DIR is None:
        LIBKTX_INCLUDE_DIR = '/usr/include'
    if LIBKTX_LIB_DIR is None:
        LIBKTX_LIB_DIR = '/usr/lib'

ffibuilder = FFI()

ffibuilder.cdef(
    """
    void free(void *ptr);

    typedef struct ktxTexture ktxTexture;
    typedef struct ktxTextureCreateInfo ktxTextureCreateInfo;
    typedef struct ktxTexture1 ktxTexture1;
    typedef struct ktxHashList ktxHashList;
    typedef struct ktxHashListEntry ktxHashListEntry;

    typedef struct {
        uint32_t error;
        ktxTexture* texture;
    } ktxTextureMixed;

    typedef struct {
        void *bytes;
        size_t size;
        int error;
    } ktxWriteToMemory;

    typedef struct {
        size_t offset;
        int error;
    } ktxImageOffset;

    // Native library
    int ktxTexture_WriteToNamedFile(ktxTexture *, const char* const);
    uint32_t ktxTexture_GetElementSize(ktxTexture *);
    uint32_t ktxTexture_GetRowPitch(ktxTexture *, uint32_t level);
    size_t ktxTexture_GetImageSize(ktxTexture *, uint32_t level);
    size_t ktxTexture_GetDataSize(ktxTexture *);
    size_t ktxTexture_GetDataSizeUncompressed(ktxTexture *);
    uint8_t *ktxTexture_GetData(ktxTexture *);
    int ktxTexture_SetImageFromMemory(ktxTexture *,
                                      uint32_t level,
                                      uint32_t layer,
                                      uint32_t faceSlice,
                                      void *src,
                                      size_t srcSize);
    int ktxTexture2_TranscodeBasis(void *, int outputFormat, int transcodeFlags);
    int ktxTexture2_DeflateZstd(void *, uint32_t compressionLevel);
    uint32_t ktxTexture2_GetOETF(void *);
    bool ktxTexture2_GetPremultipliedAlpha(void *);
    bool ktxTexture2_NeedsTranscoding(void *);

    int ktxHashList_AddKVPair(ktxHashList *, const char *key, unsigned int valueLen, const void *value);
    int ktxHashList_DeleteKVPair(ktxHashList *, const char *key);
    ktxHashListEntry *ktxHashList_Next(void *entry);

    // Glue code
    ktxTextureMixed PY_ktxTexture_CreateFromNamedFile(const char* const filename,
                                                      uint32_t create_flags);
    ktxWriteToMemory PY_ktxTexture_WriteToMemory(ktxTexture *);
    ktxImageOffset PY_ktxTexture_GetImageOffset(ktxTexture *,
                                                uint32_t level,
                                                uint32_t layer,
                                                uint32_t faceSlice);
    int PY_ktxTexture_get_classId(ktxTexture *);
    bool PY_ktxTexture_get_isArray(ktxTexture *);
    bool PY_ktxTexture_get_isCompressed(ktxTexture *);
    bool PY_ktxTexture_get_isCubemap(ktxTexture *);
    bool PY_ktxTexture_get_generateMipmaps(ktxTexture *);
    uint32_t PY_ktxTexture_get_baseWidth(ktxTexture *);
    uint32_t PY_ktxTexture_get_baseHeight(ktxTexture *);
    uint32_t PY_ktxTexture_get_baseDepth(ktxTexture *);
    uint32_t PY_ktxTexture_get_numDimensions(ktxTexture *);
    uint32_t PY_ktxTexture_get_numLevels(ktxTexture *);
    uint32_t PY_ktxTexture_get_numFaces(ktxTexture *);
    uint32_t PY_ktxTexture_get_kvDataLen(ktxTexture *);
    void *PY_ktxTexture_get_kvData(ktxTexture *);
    ktxHashListEntry *PY_ktxHashList_get_listHead(ktxHashList *list);
    ktxHashList *PY_ktxTexture_get_kvDataHead(ktxTexture *);
    ktxWriteToMemory PY_ktxHashList_FindValue(ktxHashList *, const char *key);
    ktxWriteToMemory PY_ktxHashListEntry_GetKey(ktxHashListEntry *);
    ktxWriteToMemory PY_ktxHashListEntry_GetValue(ktxHashListEntry *);

    ktxTextureMixed PY_ktxTexture1_Create(uint32_t glInternalFormat,
                                          uint32_t vkFormat,
                                          uint32_t *pDfd,
                                          uint32_t baseWidth,
                                          uint32_t baseHeight,
                                          uint32_t baseDepth,
                                          uint32_t numDimensions,
                                          uint32_t numLevels,
                                          uint32_t numLayers,
                                          uint32_t numFaces,
                                          bool isArray,
                                          bool generateMipmaps,
                                          int storageAllocation);
    uint32_t PY_ktxTexture1_get_glFormat(void *);
    uint32_t PY_ktxTexture1_get_glInternalformat(void *);
    uint32_t PY_ktxTexture1_get_glBaseInternalformat(void *);
    uint32_t PY_ktxTexture1_get_glType(void *);

    ktxTextureMixed PY_ktxTexture2_Create(uint32_t glInternalFormat,
                                          uint32_t vkFormat,
                                          uint32_t *pDfd,
                                          uint32_t baseWidth,
                                          uint32_t baseHeight,
                                          uint32_t baseDepth,
                                          uint32_t numDimensions,
                                          uint32_t numLevels,
                                          uint32_t numLayers,
                                          uint32_t numFaces,
                                          bool isArray,
                                          bool generateMipmaps,
                                          int storageAllocation);
    int PY_ktxTexture2_CompressAstcEx(void *texture,
                                      bool verbose,
                                      uint32_t threadCount,
                                      uint32_t blockDimension,
                                      uint32_t mode,
                                      uint32_t quality,
                                      bool normalMap,
                                      bool perceptual,
                                      char *inputSwizzle);
    int PY_ktxTexture2_CompressBasisEx(void *texture,
                                       bool uastc,
                                       bool verbose,
                                       bool noSSE,
                                       uint32_t threadCount,
                                       uint32_t compressionLevel,
                                       uint32_t qualityLevel,
                                       uint32_t maxEndpoints,
                                       float endpointRDOThreshold,
                                       uint32_t maxSelectors,
                                       float selectorRDOThreshold,
                                       char *inputSwizzle,
                                       bool normalMap,
                                       bool separateRGToRGB_A,
                                       bool preSwizzle,
                                       bool noEndpointRDO,
                                       bool noSelectorRDO,
                                       int uastcFlags,
                                       bool uastcRDO,
                                       float uastcRDOQualityScalar,
                                       uint32_t uastcRDODictSize,
                                       float uastcRDOMaxSmoothBlockErrorScale,
                                       float uastcRDOMaxSmoothBlockStdDev,
                                       bool uastcRDODontFavorSimplerModes,
                                       bool uastcRDONoMultithreading);
    uint32_t PY_ktxTexture2_get_vkFormat(void *);
    uint32_t PY_ktxTexture2_get_supercompressionScheme(void *);
    """
)

ffibuilder.set_source(
    "pyktx.native",
    """
    #include <ktx.h>
    #include "ktx_texture.h"
    #include "ktx_texture1.h"
    #include "ktx_texture2.h"
    """,
    include_dirs=['pyktx']
                 + ([LIBKTX_INCLUDE_DIR] if LIBKTX_INCLUDE_DIR is not None else []),
    sources=['pyktx/ktx_texture.c', 'pyktx/ktx_texture1.c', 'pyktx/ktx_texture2.c'],
    libraries=['ktx'],
    library_dirs=([LIBKTX_LIB_DIR] if LIBKTX_LIB_DIR is not None else []),
    runtime_library_dirs=(([LIBKTX_LIB_DIR] if LIBKTX_LIB_DIR is not None else []) if os.name != 'nt' else None))

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)

    if 'KTX_RUN_TESTS' in os.environ and os.environ['KTX_RUN_TESTS'] == 'ON':
        suite = unittest.TestLoader().discover(os.path.join(os.path.dirname(__file__), 'tests'))
        result = unittest.TextTestRunner(verbosity=2).run(suite)

        if not result.wasSuccessful():
            sys.exit(1)
