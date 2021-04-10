/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/**
 * @internal
 * @file info.c
 * @~English
 *
 * @brief Functions for printing information about KTX or KTX2.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

/*
 * Copyright 2019-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <io.h>
#endif

#include <ktx.h>

#include "dfdutils/dfd.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "ktxint.h"
#include "basis_sgd.h"

/*===========================================================*
 * Common Utilities for version 1 and 2.                     *
 *===========================================================*/

/**
 * @internal
 * @~English
 * @brief Prints a list of the keys & values found in a KTX file.
 *
 * @param [in]     pKvd         pointer to serialized key/value data.
 * @param [in]     kvdLen       length of the serialized key/value data.
 */
void
printKVData(ktx_uint8_t* pKvd, ktx_uint32_t kvdLen)
{
    KTX_error_code result;
    ktxHashList kvDataHead = 0;

    assert(pKvd != NULL && kvdLen > 0);

    result = ktxHashList_Deserialize(&kvDataHead,
                                     kvdLen, pKvd);
    if (result == KTX_SUCCESS) {
        if (kvDataHead == NULL) {
            fprintf(stdout, "None\n");
        } else {
            ktxHashListEntry* entry;
            for (entry = kvDataHead; entry != NULL; entry = ktxHashList_Next(entry)) {
                char* key;
                char* value; // XXX May be a binary value. How to tell?
                ktx_uint32_t keyLen, valueLen;

                ktxHashListEntry_GetKey(entry, &keyLen, &key);
                ktxHashListEntry_GetValue(entry, &valueLen, (void**)&value);
                // Keys must be NUL terminated.
                fprintf(stdout, "%s: ", key);
                // XXX How to tell if a value is binary and how to output it?
                // valueLen includes the terminating NUL, if any.
                if (value[valueLen-1] == '\0')
                    fprintf(stdout, "%s\n", value);
                else {
                    for (ktx_uint32_t i=0; i < valueLen; i++) {
                        fputc(value[i], stdout);
                    }
                    fputc('\n', stdout);
                }
            }
        }
        ktxHashList_Destruct(&kvDataHead);
    } else {
        fprintf(stdout,
                "Not enough memory to build list of key/value pairs.\n");
    }
}

void
printIdentifier(const ktx_uint8_t identifier[12])
{
    // Convert identifier for better display.
    uint32_t idlen = 0;
	char u8identifier[30];
    for (uint32_t i = 0; i < 12 && idlen < sizeof(u8identifier); i++, idlen++) {
        // Convert the angle brackets to utf-8 for better printing. The
        // conversion below only works for characters whose msb's are 10.
        if (identifier[i] == U'\xAB') {
          u8identifier[idlen++] = '\xc2';
          u8identifier[idlen] = identifier[i];
        } else if (identifier[i] == U'\xBB') {
          u8identifier[idlen++] = '\xc2';
          u8identifier[idlen] = identifier[i];
		} else if (identifier[i] < '\x20') {
			uint32_t nchars;
			switch (identifier[i]) {
			  case '\n':
				u8identifier[idlen++] = '\\';
				u8identifier[idlen] = 'n';
				break;
			  case '\r':
				u8identifier[idlen++] = '\\';
				u8identifier[idlen] = 'r';
				break;
			  default:
				nchars = snprintf(&u8identifier[idlen],
                                  sizeof(u8identifier) - idlen,
					              "\\x%02X", identifier[i]);
				idlen += nchars - 1;
			}
		} else {
          u8identifier[idlen] = identifier[i];
        }
    }
#if defined(_WIN32)
	if (_isatty(_fileno(stdout)))
	    SetConsoleOutputCP(CP_UTF8);
#endif
	fprintf(stdout, "identifier: %.*s\n", idlen, u8identifier);
}

/*===========================================================*
 * For KTX format version 1                                  *
 *===========================================================*/

/**
 * @internal
 * @~English
 * @brief Print the header fields of a KTX 1 file.
 *
 * @param [in]  pHeader pointer to the header to print.
 */
void
printKTXHeader(KTX_header* pHeader)
{
    printIdentifier(pHeader->identifier);
    fprintf(stdout, "endianness: %#x\n", pHeader->endianness);
    fprintf(stdout, "glType: %#x\n", pHeader->glType);
    fprintf(stdout, "glTypeSize: %d\n", pHeader->glTypeSize);
    fprintf(stdout, "glFormat: %#x\n", pHeader->glFormat);
    fprintf(stdout, "glInternalformat: %#x\n", pHeader->glInternalformat);
    fprintf(stdout, "glBaseInternalformat: %#x\n",
            pHeader->glBaseInternalformat);
    fprintf(stdout, "pixelWidth: %d\n", pHeader->pixelWidth);
    fprintf(stdout, "pixelHeight: %d\n", pHeader->pixelHeight);
    fprintf(stdout, "pixelDepth: %d\n", pHeader->pixelDepth);
    fprintf(stdout, "numberOfArrayElements: %d\n",
            pHeader->numberOfArrayElements);
    fprintf(stdout, "numberOfFaces: %d\n", pHeader->numberOfFaces);
    fprintf(stdout, "numberOfMipLevels: %d\n", pHeader->numberOfMipLevels);
    fprintf(stdout, "bytesOfKeyValueData: %d\n", pHeader->bytesOfKeyValueData);
}

/**
 * @internal
 * @~English
 * @brief Print information about a KTX 1 file.
 *
 * The stream's read pointer should be immediately following the header.
 *
 * @param [in]     stream  pointer to the ktxStream reading the file.
 * @param [in]     pHeader pointer to the header to print.
 */
void
printKTXInfo2(ktxStream* stream, KTX_header* pHeader)
{
    ktx_uint8_t* metadata;
    KTX_supplemental_info suppInfo;
    KTX_error_code result;

    if (pHeader->endianness == KTX_ENDIAN_REF_REV) {
        fprintf(stdout, "This file has opposite endianness to this machine. Following\n"
                        "are the converted pHeader values\n\n");
    } else {
        fprintf(stdout, "Header\n\n");
    }
    // Print first as ktxCheckHeader1_ modifies the header.
    printKTXHeader(pHeader);

    result = ktxCheckHeader1_(pHeader, &suppInfo);
    if (result != KTX_SUCCESS) {
        fprintf(stdout, "The KTX 1 file pHeader is invalid:\n");
        switch (result) {
          case KTX_FILE_DATA_ERROR:
            fprintf(stdout, "  it has invalid data such as bad glTypSize, improper dimensions,\n"
                            "improper number of faces or too many levels.\n");
            break;
          case KTX_UNSUPPORTED_TEXTURE_TYPE:
            fprintf(stdout, "  it describes a 3D array that is unsupported\n");
            break;
          default:
              ; // _ktxCheckHeader returns only the above 2 errors.
        }
        return;
    }

    if (pHeader->bytesOfKeyValueData) {
        fprintf(stdout, "\nKey/Value Data\n\n");
        metadata = malloc(pHeader->bytesOfKeyValueData);
        stream->read(stream, metadata, pHeader->bytesOfKeyValueData);
        printKVData(metadata, pHeader->bytesOfKeyValueData);
        free(metadata);
    } else {
        fprintf(stdout, "\nNo Key/Value data.\n");
    }

    uint32_t levelCount = MAX(1, pHeader->numberOfMipLevels);
    bool nonArrayCubemap;
    if (pHeader->numberOfArrayElements == 0 && pHeader->numberOfFaces == 6)
        nonArrayCubemap = true;
    else
        nonArrayCubemap = false;
    ktx_size_t dataSize = 0;
    // A note about padding: Since KTX requires a row alignment of 4 for
    // uncompressed and all block-compressed formats have block sizes that
    // are a multiple of 4, all levels and faces will also be a multiple
    // of 4 so mipPadding and facePadding will always be 0. So they are
    // ignored here.
    for (uint32_t level = 0; level < levelCount; level++) {
        ktx_uint32_t faceLodSize;
        ktx_uint32_t lodSize;
        result = stream->read(stream, &faceLodSize, sizeof(ktx_uint32_t));
        if (pHeader->endianness == KTX_ENDIAN_REF_REV)
            _ktxSwapEndian32(&faceLodSize, 1);
        if (nonArrayCubemap) {
            lodSize = faceLodSize * 6;
        } else {
            lodSize = faceLodSize;
        }
        result = stream->skip(stream, lodSize);
        dataSize += lodSize;
    }
    fprintf(stdout, "\nTotal data size = %zu\n", dataSize);
}

/**
 * @internal
 * @~English
 * @brief Print information about a KTX 1 file.
 *
 * The stream's read pointer should be at the start of the file.
 *
 * @param [in]  stream  pointer to the ktxStream reading the file.
 */
void
printKTXInfo(ktxStream* stream)
{
    KTX_header header;

    stream->read(stream, &header, KTX_HEADER_SIZE);
    printKTXInfo2(stream, &header);
}


/*===========================================================*
 * For KTX format version 2                                  *
 *===========================================================*/

extern const char* vkFormatString(VkFormat format);

extern const char * ktxSupercompressionSchemeString(ktxSupercmpScheme scheme);

/**
 * @internal
 * @~English
 * @brief Print the header fields of a KTX 2 file.
 *
 * @param [in]     pHeader      pointer to the header to print.
 */
void
printKTX2Header(KTX_header2* pHeader)
{
	printIdentifier(pHeader->identifier);
	fprintf(stdout, "vkFormat: %s\n", vkFormatString(pHeader->vkFormat));
    fprintf(stdout, "typeSize: %d\n", pHeader->typeSize);
    fprintf(stdout, "pixelWidth: %d\n", pHeader->pixelWidth);
    fprintf(stdout, "pixelHeight: %d\n", pHeader->pixelHeight);
    fprintf(stdout, "pixelDepth: %d\n", pHeader->pixelDepth);
    fprintf(stdout, "layerCount: %d\n",
            pHeader->layerCount);
    fprintf(stdout, "faceCount: %d\n", pHeader->faceCount);
    fprintf(stdout, "levelCount: %d\n", pHeader->levelCount);
    fprintf(stdout, "supercompressionScheme: %s\n",
            ktxSupercompressionSchemeString(pHeader->supercompressionScheme));
    fprintf(stdout, "dataFormatDescriptor.byteOffset: %#x\n",
            pHeader->dataFormatDescriptor.byteOffset);
    fprintf(stdout, "dataFormatDescriptor.byteLength: %d\n",
            pHeader->dataFormatDescriptor.byteLength);
    fprintf(stdout, "keyValueData.byteOffset: %#x\n", pHeader->keyValueData.byteOffset);
    fprintf(stdout, "keyValueData.byteLength: %d\n", pHeader->keyValueData.byteLength);
    fprintf(stdout, "supercompressionGlobalData.byteOffset: %#" PRIx64 "\n",
            pHeader->supercompressionGlobalData.byteOffset);
    fprintf(stdout, "supercompressionGlobalData.byteLength: %" PRId64 "\n",
            pHeader->supercompressionGlobalData.byteLength);
}

/**
 * @internal
 * @~English
 * @brief Print the level index of a KTX 2 file.
 *
 * @param [in]  levelIndex  pointer to the level index array.
 * @param [in]  numLevels   number of entries in the level index.
 */
void
printLevelIndex(ktxLevelIndexEntry levelIndex[], ktx_uint32_t numLevels)
{
    for (ktx_uint32_t level = 0; level < numLevels; level++) {
    fprintf(stdout, "Level%d.byteOffset: %#" PRIx64 "\n", level,
            levelIndex[level].byteOffset);
    fprintf(stdout, "Level%d.byteLength: %" PRId64 "\n", level,
            levelIndex[level].byteLength);
    fprintf(stdout, "Level%d.uncompressedByteLength: %" PRId64 "\n", level,
            levelIndex[level].uncompressedByteLength);
    }
}

/**
 * @internal
 * @~English
 * @brief Print Basis supercompression global data.
 *
 * @param [in]  bgd          pointer to the Basis supercompression global data.
 * @param [in]  byteLength   byte length of the data pointed to by @p bgd.
 */
void
printBasisSGDInfo(ktx_uint8_t* bgd, ktx_uint64_t byteLength,
                ktx_uint32_t numImages)
{
    ktxBasisLzGlobalHeader* bgdh = (ktxBasisLzGlobalHeader*)(bgd);

    fprintf(stdout, "endpointCount: %d\n", bgdh->endpointCount);
    fprintf(stdout, "selectorCount: %d\n", bgdh->selectorCount);
    fprintf(stdout, "endpointsByteLength: %d\n", bgdh->endpointsByteLength);
    fprintf(stdout, "selectorsByteLength: %d\n", bgdh->selectorsByteLength);
    fprintf(stdout, "tablesByteLength: %d\n", bgdh->tablesByteLength);
    fprintf(stdout, "extendedByteLength: %d\n", bgdh->extendedByteLength);

    ktxBasisLzEtc1sImageDesc* slices = (ktxBasisLzEtc1sImageDesc*)(bgd + sizeof(ktxBasisLzGlobalHeader));
    for (ktx_uint32_t i = 0; i < numImages; i++) {
        fprintf(stdout, "\nimageFlags: %#x\n", slices[i].imageFlags);
        fprintf(stdout, "rgbSliceByteLength: %d\n", slices[i].rgbSliceByteLength);
        fprintf(stdout, "rgbSliceByteOffset: %#x\n", slices[i].rgbSliceByteOffset);
        fprintf(stdout, "alphaSliceByteLength: %d\n", slices[i].alphaSliceByteLength);
        fprintf(stdout, "alphaSliceByteOffset: %#x\n", slices[i].alphaSliceByteOffset);
    }
}

/**
 * @internal
 * @~English
 * @brief Print information about a KTX 2 file.
 *
 * The stream's read pointer should be immediately following the header.
 *
 * @param [in]     stream  pointer to the ktxStream reading the file.
 * @param [in]     pHeader pointer to the header to print.
 */
void
printKTX2Info2(ktxStream* stream, KTX_header2* pHeader)
{
    ktx_uint32_t numLevels;
    ktxLevelIndexEntry* levelIndex;
    ktx_uint32_t levelIndexSize;
    ktx_uint32_t* DFD;
    ktx_uint8_t* metadata;

    fprintf(stdout, "Header\n\n");
    printKTX2Header(pHeader);

    fprintf(stdout, "\nLevel Index\n\n");
    numLevels = MAX(1, pHeader->levelCount);
    levelIndexSize = sizeof(ktxLevelIndexEntry) * numLevels;
    levelIndex = (ktxLevelIndexEntry*)malloc(levelIndexSize);
    stream->read(stream, levelIndex, levelIndexSize);
    printLevelIndex(levelIndex, numLevels);
    free(levelIndex);

    fprintf(stdout, "\nData Format Descriptor\n\n");
    DFD = (ktx_uint32_t*)malloc(pHeader->dataFormatDescriptor.byteLength);
    stream->read(stream, DFD, pHeader->dataFormatDescriptor.byteLength);
    printDFD(DFD);
    free(DFD);

    if (pHeader->keyValueData.byteLength) {
        fprintf(stdout, "\nKey/Value Data\n\n");
        metadata = malloc(pHeader->keyValueData.byteLength);
        stream->read(stream, metadata, pHeader->keyValueData.byteLength);
        printKVData(metadata, pHeader->keyValueData.byteLength);
        free(metadata);
    } else {
        fprintf(stdout, "\nNo Key/Value data.\n");
    }

    if (pHeader->supercompressionGlobalData.byteOffset != 0
        && pHeader->supercompressionGlobalData.byteLength != 0) {
        if (pHeader->supercompressionScheme == KTX_SS_BASIS_LZ) {
            ktx_uint8_t* sgd = malloc(pHeader->supercompressionGlobalData.byteLength);
            stream->setpos(stream, pHeader->supercompressionGlobalData.byteOffset);
            stream->read(stream, sgd, pHeader->supercompressionGlobalData.byteLength);
            //
            // Calculate number of images
            //
            uint32_t layersFaces = MAX(pHeader->layerCount, 1) * pHeader->faceCount;
            uint32_t layerPixelDepth = MAX(pHeader->pixelDepth, 1);
            for(uint32_t level = 1; level < MAX(pHeader->levelCount, 1); level++)
                layerPixelDepth += MAX(MAX(pHeader->pixelDepth, 1) >> level, 1U);
            // NOTA BENE: faceCount * layerPixelDepth is only reasonable because
            // faceCount and depth can't both be > 1. I.e there are no 3d cubemaps.
            uint32_t numImages = layersFaces * layerPixelDepth;
            fprintf(stdout, "\nBasis Supercompression Global Data\n\n");
            printBasisSGDInfo(sgd, pHeader->supercompressionGlobalData.byteLength, numImages);
        } else {
            fprintf(stdout, "\nUnrecognized supercompressionScheme.");
        }
    }
}

/**
 * @internal
 * @~English
 * @brief Print information about a KTX 2 file.
 *
 * The stream's read pointer should be at the start of the file.
 *
 * @param [in]     stream     pointer to the ktxStream reading the file.
 */
void
printKTX2Info(ktxStream* stream)
{
    KTX_header2 header;

    stream->read(stream, &header, KTX2_HEADER_SIZE);
    printKTX2Info2(stream, &header);
}

/*===========================================================*
 * Functions that determining format and invoke print func's *
 *===========================================================*/

/**
 * @internal
 * @~English
 * @brief Print information about a KTX file.
 *
 * Determine the format of the KTX file and print appropriate information.
 * The stream's read pointer should be at the start of the file.
 *
 * @param [in]  stream  pointer to the ktxStream reading the file.
 */
KTX_error_code
ktxPrintInfoForStream(ktxStream* stream)
{
    enum { KTX, KTX2 } fileType;
    ktx_uint8_t ktx_ident_ref[12] = KTX_IDENTIFIER_REF;
    ktx_uint8_t ktx2_ident_ref[12] = KTX2_IDENTIFIER_REF;
    union {
        KTX_header ktx;
        KTX_header2 ktx2;
    } header;
    KTX_error_code result;

    assert(stream != NULL);

    result = stream->read(stream, &header, sizeof(ktx2_ident_ref));
    if (result == KTX_SUCCESS) {
        // Compare identifier, is this a KTX  or KTX2 file?
        if (!memcmp(header.ktx.identifier, ktx_ident_ref, 12)) {
                fileType = KTX;
        } else if (!memcmp(header.ktx2.identifier, ktx2_ident_ref, 12)) {
                fileType = KTX2;
        } else {
                return KTX_UNKNOWN_FILE_FORMAT;
        }
        if (fileType == KTX) {
            // Read rest of header.
            result = stream->read(stream, &header.ktx.endianness,
                                  KTX_HEADER_SIZE - sizeof(ktx_ident_ref));
            printKTXInfo2(stream, &header.ktx);
        } else {
           // Read rest of header.
           result = stream->read(stream, &header.ktx2.vkFormat,
                                 KTX2_HEADER_SIZE - sizeof(ktx2_ident_ref));
           printKTX2Info2(stream, &header.ktx2);
        }
    }
    return result;
}

/**
 * @~English
 * @brief Print information about a KTX file on a stdioStream.
 *
 * Determine the format of the KTX file and print appropriate information.
 * The stdioStream's read pointer should be at the start of the file.
 *
 * @param [in]  stream  pointer to the ktxStream reading the file.
 */
KTX_error_code
ktxPrintInfoForStdioStream(FILE* stdioStream)
{
    KTX_error_code result;
    ktxStream stream;

    if (stdioStream == NULL)
        return KTX_INVALID_VALUE;

    result = ktxFileStream_construct(&stream, stdioStream, KTX_FALSE);
    if (result == KTX_SUCCESS)
        result = ktxPrintInfoForStream(&stream);
    return result;

    return KTX_SUCCESS;
}

/**
 * @~English
 * @brief Print information about a named KTX file.
 *
 * Determine the format of the KTX file and print appropriate information.
 *
 * @param [in]  filename  name of the KTX file.
 */
KTX_error_code
ktxPrintInfoForNamedFile(const char* const filename)
{
    return KTX_SUCCESS;
}

/**
 * @~English
 * @brief Print information about a KTX file in memory.
 *
 * Determine the format of the KTX file and print appropriate information.
 *
 * @param [in]  bytes   pointer to the memory holding the KTX file.
 * @param [in]  size    length of the KTX file in bytes.
 */
KTX_error_code
ktxPrintInfoForMemory(const ktx_uint8_t* bytes, ktx_size_t size)
{
    KTX_error_code result;
    ktxStream stream;

    result = ktxMemStream_construct_ro(&stream, bytes, size);
    if (result == KTX_SUCCESS)
        result = ktxPrintInfoForStream(&stream);
    return result;
    return KTX_SUCCESS;
}
