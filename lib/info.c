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
#include "filestream.h"
#include "memstream.h"
#include "ktxint.h"
#include "basis_sgd.h"
#include "unused.h"

/*===========================================================*
 * Common Utilities for version 1 and 2.                     *
 *===========================================================*/

enum {
    // These constraints are not mandated by the spec and only used as a
    // reasonable upper limit to stop parsing garbage data during print
    MAX_NUM_KVD_ENTRIES = 100,
    MAX_NUM_LEVELS = 64,
};

/** @internal */
#define LENGTH_OF_INDENT(INDENT) ((base_indent + INDENT) * indent_width)

/** @internal */
#define PRINT_INDENT(INDENT, FMT, ...) {                              \
        printf("%*s" FMT, LENGTH_OF_INDENT(INDENT), "", __VA_ARGS__); \
    }

/** @internal */
#define PRINT_INDENT_NOARG(INDENT, FMT) {                \
        printf("%*s" FMT, LENGTH_OF_INDENT(INDENT), ""); \
    }

/** @internal */
static void printFlagBitsJSON(uint32_t indent, const char* nl, uint32_t flags, const char*(*toStringFn)(uint32_t, bool)) {
    bool first = true;
    for (uint32_t bit_index = 0; bit_index < 32; ++bit_index) {
        uint32_t bit_mask = 1u << bit_index;
        bool bit_value = (bit_mask & (uint32_t) flags) != 0;

        const char* str = toStringFn(bit_index, bit_value);
        if (str) {
            printf("%s%s%*s\"%s\"", first ? "" : ",", first ? "" : nl, indent, "", str);
            first = false;
        } else if (bit_value) {
            printf("%s%s%*s%u", first ? "" : ",", first ? "" : nl, indent, "", bit_mask);
            first = false;
        }
    }
    if (!first)
        printf("%s", nl);
}

/** @internal */
bool isKnownKeyValueUINT32(const char* key) {
    if (strcmp(key, "KTXdxgiFormat__") == 0)
        return true;
    if (strcmp(key, "KTXmetalPixelFormat") == 0)
        return true;

    return false;
}

/** @internal */
bool isKnownKeyValueString(const char* key) {
    if (strcmp(key, "KTXorientation") == 0)
        return true;
    if (strcmp(key, "KTXswizzle") == 0)
        return true;
    if (strcmp(key, "KTXwriter") == 0)
        return true;
    if (strcmp(key, "KTXwriterScParams") == 0)
        return true;
    if (strcmp(key, "KTXastcDecodeMode") == 0)
        return true;

    return false;
}

/** @internal */
bool isKnownKeyValue(const char* key) {
    if (isKnownKeyValueUINT32(key))
        return true;
    if (isKnownKeyValueString(key))
        return true;
    if (strcmp(key, "KTXglFormat") == 0)
        return true;
    if (strcmp(key, "KTXanimData") == 0)
        return true;
    if (strcmp(key, "KTXcubemapIncomplete") == 0)
        return true;
    return false;
}

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

    result = ktxHashList_Deserialize(&kvDataHead, kvdLen, pKvd);
    if (result != KTX_SUCCESS) {
        fprintf(stdout, "Failed to parse or not enough memory to build list of key/value pairs.\n");
        return;
    }

    if (kvDataHead == NULL)
        return;

    int entryIndex = 0;
    ktxHashListEntry* entry = kvDataHead;
    for (; entry != NULL && entryIndex < MAX_NUM_KVD_ENTRIES; entry = ktxHashList_Next(entry), ++entryIndex) {
        char* key;
        char* value;
        ktx_uint32_t keyLen, valueLen;

        ktxHashListEntry_GetKey(entry, &keyLen, &key);
        ktxHashListEntry_GetValue(entry, &valueLen, (void**)&value);
        // Keys must be NUL terminated.
        fprintf(stdout, "%s:", key);
        if (!value) {
            fprintf(stdout, " null\n");

        } else {
            if (strcmp(key, "KTXglFormat") == 0) {
                if (valueLen == 3 * sizeof(ktx_uint32_t)) {
                    ktx_uint32_t glInternalformat = *(const ktx_uint32_t*) (value + 0);
                    ktx_uint32_t glFormat = *(const ktx_uint32_t*) (value + 4);
                    ktx_uint32_t glType = *(const ktx_uint32_t*) (value + 8);
                    fprintf(stdout, "\n");
                    fprintf(stdout, "    glInternalformat: 0x%08X\n", glInternalformat);
                    fprintf(stdout, "    glFormat: 0x%08X\n", glFormat);
                    fprintf(stdout, "    glType: 0x%08X\n", glType);
                }

            } else if (strcmp(key, "KTXanimData") == 0) {
                if (valueLen == 3 * sizeof(ktx_uint32_t)) {
                    ktx_uint32_t duration = *(const ktx_uint32_t*) (value + 0);
                    ktx_uint32_t timescale = *(const ktx_uint32_t*) (value + 4);
                    ktx_uint32_t loopCount = *(const ktx_uint32_t*) (value + 8);
                    fprintf(stdout, "\n");
                    fprintf(stdout, "    duration: %u\n", duration);
                    fprintf(stdout, "    timescale: %u\n", timescale);
                    fprintf(stdout, "    loopCount: %u%s\n", loopCount, loopCount == 0 ? " (infinite)" : "");
                }

            } else if (strcmp(key, "KTXcubemapIncomplete") == 0) {
                if (valueLen == sizeof(ktx_uint8_t)) {
                    ktx_uint8_t faces = *value;
                    fprintf(stdout, "\n");
                    fprintf(stdout, "    positiveX: %s\n", faces & 1u << 0u ? "true" : "false");
                    fprintf(stdout, "    negativeX: %s\n", faces & 1u << 1u ? "true" : "false");
                    fprintf(stdout, "    positiveY: %s\n", faces & 1u << 2u ? "true" : "false");
                    fprintf(stdout, "    negativeY: %s\n", faces & 1u << 3u ? "true" : "false");
                    fprintf(stdout, "    positiveZ: %s\n", faces & 1u << 4u ? "true" : "false");
                    fprintf(stdout, "    negativeZ: %s\n", faces & 1u << 5u ? "true" : "false");
                }

            } else if (isKnownKeyValueUINT32(key)) {
                if (valueLen == sizeof(ktx_uint32_t)) {
                    ktx_uint32_t number = *(const ktx_uint32_t*) value;
                    fprintf(stdout, " %u\n", number);
                }

            } else if (isKnownKeyValueString(key)) {
                if (value[valueLen-1] == '\0') {
                    fprintf(stdout, " %s\n", value);
                }

            } else {
                fprintf(stdout, " [");
                for (ktx_uint32_t i = 0; i < valueLen; i++)
                    fprintf(stdout, "%d%s", (int) value[i], i + 1 == valueLen ? "" : ", ");
                fprintf(stdout, "]\n");
            }
        }
    }

    ktxHashList_Destruct(&kvDataHead);
}

/**
 * @internal
 * @~English
 * @brief Prints a list of the keys & values found in a KTX2 file.
 *
 * @param [in]     pKvd         pointer to serialized key/value data.
 * @param [in]     kvdLen       length of the serialized key/value data.
 * @param [in]     base_indent  The number of indentations to include at the front of every line
 * @param [in]     indent_width The number of spaces to add with each nested scope
 * @param [in]     minified     Specifies whether the JSON output should be minified
 */
void
printKVDataJSON(ktx_uint8_t* pKvd, ktx_uint32_t kvdLen, ktx_uint32_t base_indent, ktx_uint32_t indent_width, bool minified)
{
    const char* space = minified ? "" : " ";
    const char* nl = minified ? "" : "\n";

    KTX_error_code result;
    ktxHashList kvDataHead = 0;

    assert(pKvd != NULL && kvdLen > 0);

    result = ktxHashList_Deserialize(&kvDataHead, kvdLen, pKvd);
    if (result != KTX_SUCCESS) {
        // Logging while printing JSON is not possible, we rely on the validation step to provide meaningful errors
        // fprintf(stdout, "Failed to parse or not enough memory to build list of key/value pairs.\n");
        return;
    }

    if (kvDataHead == NULL)
        return;

    int entryIndex = 0;
    ktxHashListEntry* entry = kvDataHead;
    bool firstPrint = true; // Marks if the first print did not occur yet (first print != first entry)
    for (; entry != NULL && entryIndex < MAX_NUM_KVD_ENTRIES; entry = ktxHashList_Next(entry), ++entryIndex) {
        char* key;
        char* value;
        ktx_uint32_t keyLen, valueLen;

        ktxHashListEntry_GetKey(entry, &keyLen, &key);
        ktxHashListEntry_GetValue(entry, &valueLen, (void**)&value);
        // Keys must be NUL terminated.
        if (!value) {
            if (!isKnownKeyValue(key)) {
                // Known keys are not be printed with null
                if (!firstPrint)
                    fprintf(stdout, ",%s", nl);
                firstPrint = false;
                PRINT_INDENT(0, "\"%s\":%snull", key, space)
            }
        } else {
            if (strcmp(key, "KTXglFormat") == 0) {
                if (valueLen == 3 * sizeof(ktx_uint32_t)) {
                    if (!firstPrint)
                        fprintf(stdout, ",%s", nl);
                    firstPrint = false;
                    ktx_uint32_t glInternalformat = *(const ktx_uint32_t*) (value + 0);
                    ktx_uint32_t glFormat = *(const ktx_uint32_t*) (value + 4);
                    ktx_uint32_t glType = *(const ktx_uint32_t*) (value + 8);
                    PRINT_INDENT(0, "\"%s\":%s{%s", key, space, nl)
                    PRINT_INDENT(1, "\"glInternalformat\":%s%u,%s", space, glInternalformat, nl)
                    PRINT_INDENT(1, "\"glFormat\":%s%u,%s", space, glFormat, nl)
                    PRINT_INDENT(1, "\"glType\":%s%u%s", space, glType, nl)
                    PRINT_INDENT_NOARG(0, "}")
                }
            } else if (strcmp(key, "KTXanimData") == 0) {
                if (valueLen == 3 * sizeof(ktx_uint32_t)) {
                    if (!firstPrint)
                        fprintf(stdout, ",%s", nl);
                    firstPrint = false;
                    ktx_uint32_t duration = *(const ktx_uint32_t*) (value + 0);
                    ktx_uint32_t timescale = *(const ktx_uint32_t*) (value + 4);
                    ktx_uint32_t loopCount = *(const ktx_uint32_t*) (value + 8);
                    PRINT_INDENT(0, "\"%s\":%s{%s", key, space, nl)
                    PRINT_INDENT(1, "\"duration\":%s%u,%s", space, duration, nl)
                    PRINT_INDENT(1, "\"timescale\":%s%u,%s", space, timescale, nl)
                    PRINT_INDENT(1, "\"loopCount\":%s%u%s", space, loopCount, nl)
                    PRINT_INDENT_NOARG(0, "}")
                }
            } else if (strcmp(key, "KTXcubemapIncomplete") == 0) {
                if (valueLen == sizeof(ktx_uint8_t)) {
                    if (!firstPrint)
                        fprintf(stdout, ",%s", nl);
                    firstPrint = false;
                    ktx_uint8_t faces = *value;
                    PRINT_INDENT(0, "\"%s\":%s{%s", key, space, nl)
                    PRINT_INDENT(1, "\"positiveX\":%s%s,%s", space, faces & 1u << 0u ? "true" : "false", nl)
                    PRINT_INDENT(1, "\"negativeX\":%s%s,%s", space, faces & 1u << 1u ? "true" : "false", nl)
                    PRINT_INDENT(1, "\"positiveY\":%s%s,%s", space, faces & 1u << 2u ? "true" : "false", nl)
                    PRINT_INDENT(1, "\"negativeY\":%s%s,%s", space, faces & 1u << 3u ? "true" : "false", nl)
                    PRINT_INDENT(1, "\"positiveZ\":%s%s,%s", space, faces & 1u << 4u ? "true" : "false", nl)
                    PRINT_INDENT(1, "\"negativeZ\":%s%s%s", space, faces & 1u << 5u ? "true" : "false", nl)
                    PRINT_INDENT_NOARG(0, "}")
                }
            } else if (isKnownKeyValueUINT32(key)) {
                if (valueLen == sizeof(ktx_uint32_t)) {
                    if (!firstPrint)
                        fprintf(stdout, ",%s", nl);
                    firstPrint = false;
                    ktx_uint32_t number = *(const ktx_uint32_t*) value;
                    PRINT_INDENT(0, "\"%s\":%s%u", key, space, number)
                }
            } else if (isKnownKeyValueString(key)) {
                if (value[valueLen-1] == '\0') {
                    if (!firstPrint)
                        fprintf(stdout, ",%s", nl);
                    firstPrint = false;
                    PRINT_INDENT(0, "\"%s\":%s\"%s\"", key, space, value)
                }
            } else {
                if (!firstPrint)
                    fprintf(stdout, ",%s", nl);
                firstPrint = false;
                PRINT_INDENT(0, "\"%s\":%s[", key, space)
                for (ktx_uint32_t i = 0; i < valueLen; i++)
                    fprintf(stdout, "%d%s", (int) value[i], i + 1 == valueLen ? "" : ", ");
                fprintf(stdout, "]");
            }
        }
    }
    fprintf(stdout, "%s", nl);

    ktxHashList_Destruct(&kvDataHead);
}

/**
 * @internal
 * @~English
 * @brief Print the KTX 1/2 file identifier.
 *
 * @param [in]  identifier
 * @param [in]  json specifies if "\x1A" should be escaped as "\u001A" to not break most json tools
 */
void
printIdentifier(const ktx_uint8_t identifier[12], bool json)
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
                                  json ? "\\u%04X" : "\\x%02X",
                                  identifier[i]);
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
    fprintf(stdout, "%.*s", idlen, u8identifier);
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
    fprintf(stdout, "identifier: ");
    printIdentifier(pHeader->identifier, false);
    fprintf(stdout, "\n");
    fprintf(stdout, "endianness: %#x\n", pHeader->endianness);
    fprintf(stdout, "glType: %#x\n", pHeader->glType);
    fprintf(stdout, "glTypeSize: %u\n", pHeader->glTypeSize);
    fprintf(stdout, "glFormat: %#x\n", pHeader->glFormat);
    fprintf(stdout, "glInternalformat: %#x\n", pHeader->glInternalformat);
    fprintf(stdout, "glBaseInternalformat: %#x\n",
            pHeader->glBaseInternalformat);
    fprintf(stdout, "pixelWidth: %u\n", pHeader->pixelWidth);
    fprintf(stdout, "pixelHeight: %u\n", pHeader->pixelHeight);
    fprintf(stdout, "pixelDepth: %u\n", pHeader->pixelDepth);
    fprintf(stdout, "numberOfArrayElements: %u\n",
            pHeader->numberOfArrayElements);
    fprintf(stdout, "numberOfFaces: %u\n", pHeader->numberOfFaces);
    fprintf(stdout, "numberOfMipLevels: %u\n", pHeader->numberOfMipLevels);
    fprintf(stdout, "bytesOfKeyValueData: %u\n", pHeader->bytesOfKeyValueData);
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
            fprintf(stdout, "  it has invalid data such as bad glTypeSize, improper dimensions,\n"
                            "improper number of faces or too many levels.\n");
            break;
          case KTX_UNSUPPORTED_FEATURE:
            fprintf(stdout, "  it describes an unsupported feature or format\n");
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
    ktx_uint64_t dataSize = 0;
    // A note about padding: Since KTX requires a row alignment of 4 for
    // uncompressed and all block-compressed formats have block sizes that
    // are a multiple of 4, all levels and faces will also be a multiple
    // of 4 so mipPadding and facePadding will always be 0. So they are
    // ignored here.
    fprintf(stdout, "\nData Sizes (bytes)\n------------------\n");
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
        fprintf(stdout, "Level %u: %u\n", level, lodSize);
    }
    fprintf(stdout, "\nTotal: %" PRId64 "\n", dataSize);
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

extern const char* ktxSupercompressionSchemeString(ktxSupercmpScheme scheme);

const char* ktxBUImageFlagsBitString(ktx_uint32_t bit_index, bool bit_value);

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
    fprintf(stdout, "identifier: ");
    printIdentifier(pHeader->identifier, false);
    fprintf(stdout, "\n");
    const char* vkFormatStr = vkFormatString(pHeader->vkFormat);
    if (strcmp(vkFormatStr, "VK_UNKNOWN_FORMAT") == 0)
        fprintf(stdout, "vkFormat: 0x%08X\n", (uint32_t) pHeader->vkFormat);
    else
        fprintf(stdout, "vkFormat: %s\n", vkFormatStr);
    fprintf(stdout, "typeSize: %u\n", pHeader->typeSize);
    fprintf(stdout, "pixelWidth: %u\n", pHeader->pixelWidth);
    fprintf(stdout, "pixelHeight: %u\n", pHeader->pixelHeight);
    fprintf(stdout, "pixelDepth: %u\n", pHeader->pixelDepth);
    fprintf(stdout, "layerCount: %u\n",
            pHeader->layerCount);
    fprintf(stdout, "faceCount: %u\n", pHeader->faceCount);
    fprintf(stdout, "levelCount: %u\n", pHeader->levelCount);
    const char* scSchemeStr = ktxSupercompressionSchemeString(pHeader->supercompressionScheme);
    if (strcmp(scSchemeStr, "Invalid scheme value") == 0)
        fprintf(stdout, "supercompressionScheme: Invalid scheme (0x%X)\n", (uint32_t) pHeader->supercompressionScheme);
    else if (strcmp(scSchemeStr, "Vendor or reserved scheme") == 0)
        fprintf(stdout, "supercompressionScheme: Vendor or reserved scheme (0x%X)\n", (uint32_t) pHeader->supercompressionScheme);
    else
        fprintf(stdout, "supercompressionScheme: %s\n", scSchemeStr);
    fprintf(stdout, "dataFormatDescriptor.byteOffset: %#x\n",
            pHeader->dataFormatDescriptor.byteOffset);
    fprintf(stdout, "dataFormatDescriptor.byteLength: %u\n",
            pHeader->dataFormatDescriptor.byteLength);
    fprintf(stdout, "keyValueData.byteOffset: %#x\n", pHeader->keyValueData.byteOffset);
    fprintf(stdout, "keyValueData.byteLength: %u\n", pHeader->keyValueData.byteLength);
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
    numLevels = MIN(MAX_NUM_LEVELS, numLevels); // Print at most 64 levels to stop parsing garbage
    for (ktx_uint32_t level = 0; level < numLevels; level++) {
    fprintf(stdout, "Level%u.byteOffset: %#" PRIx64 "\n", level,
            levelIndex[level].byteOffset);
    fprintf(stdout, "Level%u.byteLength: %" PRId64 "\n", level,
            levelIndex[level].byteLength);
    fprintf(stdout, "Level%u.uncompressedByteLength: %" PRId64 "\n", level,
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
    if (byteLength < sizeof(ktxBasisLzGlobalHeader))
        return;

    fprintf(stdout, "endpointCount: %u\n", bgdh->endpointCount);
    fprintf(stdout, "selectorCount: %u\n", bgdh->selectorCount);
    fprintf(stdout, "endpointsByteLength: %u\n", bgdh->endpointsByteLength);
    fprintf(stdout, "selectorsByteLength: %u\n", bgdh->selectorsByteLength);
    fprintf(stdout, "tablesByteLength: %u\n", bgdh->tablesByteLength);
    fprintf(stdout, "extendedByteLength: %u\n", bgdh->extendedByteLength);

    ktxBasisLzEtc1sImageDesc* slices = (ktxBasisLzEtc1sImageDesc*)(bgd + sizeof(ktxBasisLzGlobalHeader));
    for (ktx_uint32_t i = 0; i < numImages; i++) {
        if (byteLength < (i + 1) * sizeof(ktxBasisLzEtc1sImageDesc) + sizeof(ktxBasisLzGlobalHeader))
            break;

        fprintf(stdout, "\nimageFlags: %#x\n", slices[i].imageFlags);
        fprintf(stdout, "rgbSliceByteLength: %u\n", slices[i].rgbSliceByteLength);
        fprintf(stdout, "rgbSliceByteOffset: %#x\n", slices[i].rgbSliceByteOffset);
        fprintf(stdout, "alphaSliceByteLength: %u\n", slices[i].alphaSliceByteLength);
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
KTX_error_code
printKTX2Info2(ktxStream* stream, KTX_header2* pHeader)
{
    const bool hasDFD =
            pHeader->dataFormatDescriptor.byteOffset != 0 &&
            pHeader->dataFormatDescriptor.byteLength != 0;
    const bool hasKVD =
            pHeader->keyValueData.byteOffset != 0 &&
            pHeader->keyValueData.byteLength != 0;
    const bool hasSGD =
            pHeader->supercompressionGlobalData.byteOffset != 0 &&
            pHeader->supercompressionGlobalData.byteLength != 0;

    ktx_uint32_t numLevels;
    ktxLevelIndexEntry* levelIndex;
    ktx_uint32_t levelIndexSize;
    KTX_error_code ec = KTX_SUCCESS;

    fprintf(stdout, "Header\n\n");
    printKTX2Header(pHeader);

    fprintf(stdout, "\nLevel Index\n\n");
    numLevels = MAX(1, pHeader->levelCount);
    levelIndexSize = sizeof(ktxLevelIndexEntry) * numLevels;
    levelIndex = (ktxLevelIndexEntry*)malloc(levelIndexSize);
    if (levelIndex == NULL)
        return KTX_OUT_OF_MEMORY;
    ec = stream->read(stream, levelIndex, levelIndexSize);
    if (ec != KTX_SUCCESS) {
        free(levelIndex);
        return ec;
    }
    printLevelIndex(levelIndex, numLevels);
    free(levelIndex);

    if (hasDFD) {
        fprintf(stdout, "\nData Format Descriptor\n\n");
        ktx_uint32_t* dfd = (ktx_uint32_t*)malloc(pHeader->dataFormatDescriptor.byteLength);
        if (dfd == NULL)
            return KTX_OUT_OF_MEMORY;
        ec = stream->read(stream, dfd, pHeader->dataFormatDescriptor.byteLength);
        if (ec != KTX_SUCCESS) {
            free(dfd);
            return ec;
        }
        if (*dfd != pHeader->dataFormatDescriptor.byteLength) {
            free(dfd);
            return KTX_FILE_DATA_ERROR;
        }
        printDFD(dfd, pHeader->dataFormatDescriptor.byteLength);
        free(dfd);
    }

    if (hasKVD) {
        fprintf(stdout, "\nKey/Value Data\n\n");
        ktx_uint8_t* kvd = malloc(pHeader->keyValueData.byteLength);
        if (kvd == NULL)
            return KTX_OUT_OF_MEMORY;
        ec = stream->read(stream, kvd, pHeader->keyValueData.byteLength);
        if (ec != KTX_SUCCESS) {
            free(kvd);
            return ec;
        }
        printKVData(kvd, pHeader->keyValueData.byteLength);
        free(kvd);
    } else {
        fprintf(stdout, "\nNo Key/Value data.\n");
    }

    if (hasSGD) {
        if (pHeader->supercompressionScheme == KTX_SS_BASIS_LZ) {
            ktx_uint8_t* sgd = malloc(pHeader->supercompressionGlobalData.byteLength);
            if (sgd == NULL)
                return KTX_OUT_OF_MEMORY;
            ec = stream->setpos(stream, pHeader->supercompressionGlobalData.byteOffset);
            if (ec != KTX_SUCCESS) {
                free(sgd);
                return ec;
            }
            ec = stream->read(stream, sgd, pHeader->supercompressionGlobalData.byteLength);
            if (ec != KTX_SUCCESS) {
                free(sgd);
                return ec;
            }
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
            free(sgd);
        } else {
            fprintf(stdout, "\nUnrecognized supercompressionScheme.\n");
        }
    }

    return ec;
}

/**
 * @internal
 * @~English
 * @brief Print information about a KTX 2 file.
 *
 * The stream's read pointer should be immediately following the header.
 *
 * @param [in]     stream       pointer to the ktxStream reading the file.
 * @param [in]     pHeader      pointer to the header to print.
 * @param [in]     base_indent  The number of indentations to include at the front of every line
 * @param [in]     indent_width The number of spaces to add with each nested scope
 * @param [in]     minified     Specifies whether the JSON output should be minified
 */
KTX_error_code
printKTX2Info2JSON(ktxStream* stream, KTX_header2* pHeader, ktx_uint32_t base_indent, ktx_uint32_t indent_width, bool minified)
{
    if (minified) {
        base_indent = 0;
        indent_width = 0;
    }
    const char* space = minified ? "" : " ";
    const char* nl = minified ? "" : "\n";

    const bool hasDFD =
            pHeader->dataFormatDescriptor.byteOffset != 0 &&
            pHeader->dataFormatDescriptor.byteLength != 0;
    const bool hasKVD =
            pHeader->keyValueData.byteOffset != 0 &&
            pHeader->keyValueData.byteLength != 0;
    const bool hasSGD =
            pHeader->supercompressionGlobalData.byteOffset != 0 &&
            pHeader->supercompressionGlobalData.byteLength != 0;

    ktx_uint32_t numLevels;
    ktxLevelIndexEntry* levelIndex;
    ktx_uint32_t levelIndexSize;

    KTX_error_code ec = KTX_SUCCESS;

    PRINT_INDENT(0, "\"header\":%s{%s", space, nl)
    PRINT_INDENT(1, "\"identifier\":%s\"", space)
    printIdentifier(pHeader->identifier, true);
    printf("\",%s", nl);
    const char* vkFormatStr = vkFormatString(pHeader->vkFormat);
    if (strcmp(vkFormatStr, "VK_UNKNOWN_FORMAT") == 0)
        PRINT_INDENT(1, "\"vkFormat\":%s%u,%s", space, (uint32_t) pHeader->vkFormat, nl)
    else
        PRINT_INDENT(1, "\"vkFormat\":%s\"%s\",%s", space, vkFormatStr, nl)
    PRINT_INDENT(1, "\"typeSize\":%s%u,%s", space, pHeader->typeSize, nl);
    PRINT_INDENT(1, "\"pixelWidth\":%s%u,%s", space, pHeader->pixelWidth, nl);
    PRINT_INDENT(1, "\"pixelHeight\":%s%u,%s", space, pHeader->pixelHeight, nl);
    PRINT_INDENT(1, "\"pixelDepth\":%s%u,%s", space, pHeader->pixelDepth, nl);
    PRINT_INDENT(1, "\"layerCount\":%s%u,%s", space, pHeader->layerCount, nl);
    PRINT_INDENT(1, "\"faceCount\":%s%u,%s", space, pHeader->faceCount, nl);
    PRINT_INDENT(1, "\"levelCount\":%s%u,%s", space, pHeader->levelCount, nl);
    const char* scSchemeStr = ktxSupercompressionSchemeString(pHeader->supercompressionScheme);
    if (strcmp(scSchemeStr, "Invalid scheme value") == 0 || strcmp(scSchemeStr, "Vendor or reserved scheme") == 0)
        PRINT_INDENT(1, "\"supercompressionScheme\":%s%u%s", space, (uint32_t) pHeader->supercompressionScheme, nl)
    else
        PRINT_INDENT(1, "\"supercompressionScheme\":%s\"%s\"%s", space, scSchemeStr, nl)
    PRINT_INDENT_NOARG(0, "}")

    numLevels = MAX(1, pHeader->levelCount);
    levelIndexSize = sizeof(ktxLevelIndexEntry) * numLevels;
    levelIndex = (ktxLevelIndexEntry*)malloc(levelIndexSize);
    if (levelIndex == NULL)
        return KTX_OUT_OF_MEMORY;
    ec = stream->read(stream, levelIndex, levelIndexSize);
    if (ec != KTX_SUCCESS) {
        printf("%s", nl);
        free(levelIndex);
        return ec;
    }

    printf(",%s", nl);
    PRINT_INDENT(0, "\"index\":%s{%s", space, nl)

    PRINT_INDENT(1, "\"dataFormatDescriptor\":%s{%s", space, nl)
    PRINT_INDENT(2, "\"byteOffset\":%s%u,%s", space, pHeader->dataFormatDescriptor.byteOffset, nl);
    PRINT_INDENT(2, "\"byteLength\":%s%u%s", space, pHeader->dataFormatDescriptor.byteLength, nl);
    PRINT_INDENT(1, "},%s", nl)
    PRINT_INDENT(1, "\"keyValueData\":%s{%s", space, nl)
    PRINT_INDENT(2, "\"byteOffset\":%s%u,%s", space, pHeader->keyValueData.byteOffset, nl);
    PRINT_INDENT(2, "\"byteLength\":%s%u%s", space, pHeader->keyValueData.byteLength, nl);
    PRINT_INDENT(1, "},%s", nl)
    PRINT_INDENT(1, "\"supercompressionGlobalData\":%s{%s", space, nl)
    PRINT_INDENT(2, "\"byteOffset\":%s%" PRId64 ",%s", space, pHeader->supercompressionGlobalData.byteOffset, nl);
    PRINT_INDENT(2, "\"byteLength\":%s%" PRId64 "%s", space, pHeader->supercompressionGlobalData.byteLength, nl);
    PRINT_INDENT(1, "},%s", nl)

    PRINT_INDENT(1, "\"levels\":%s[%s", space, nl)
    numLevels = MIN(MAX_NUM_LEVELS, numLevels); // Print at most 64 levels to stop parsing garbage
    for (ktx_uint32_t level = 0; level < numLevels; level++) {
        PRINT_INDENT(2, "{%s", nl);
        PRINT_INDENT(3, "\"byteOffset\":%s%" PRId64 ",%s", space, levelIndex[level].byteOffset, nl);
        PRINT_INDENT(3, "\"byteLength\":%s%" PRId64 ",%s", space, levelIndex[level].byteLength, nl);
        PRINT_INDENT(3, "\"uncompressedByteLength\":%s%" PRId64 "%s", space, levelIndex[level].uncompressedByteLength, nl);
        PRINT_INDENT(2, "}%s%s", level + 1 == numLevels ? "" : ",", nl);
    }
    PRINT_INDENT(1, "]%s", nl) // End of levels

    free(levelIndex);
    PRINT_INDENT_NOARG(0, "}") // End of index

    if (hasDFD) {
        ktx_uint32_t* dfd = (ktx_uint32_t*)malloc(pHeader->dataFormatDescriptor.byteLength);
        if (dfd == NULL)
            return KTX_OUT_OF_MEMORY;
        ec = stream->read(stream, dfd, pHeader->dataFormatDescriptor.byteLength);
        if (ec != KTX_SUCCESS) {
            printf("%s", nl);
            free(dfd);
            return ec;
        }
        printf(",%s", nl);
        PRINT_INDENT(0, "\"dataFormatDescriptor\":%s{%s", space, nl)
        printDFDJSON(dfd, pHeader->dataFormatDescriptor.byteLength, base_indent + 1, indent_width, minified);
        free(dfd);
        PRINT_INDENT_NOARG(0, "}")
    }

    if (hasKVD) {
        ktx_uint8_t* kvd = malloc(pHeader->keyValueData.byteLength);
        if (kvd == NULL)
            return KTX_OUT_OF_MEMORY;
        ec = stream->read(stream, kvd, pHeader->keyValueData.byteLength);
        if (ec != KTX_SUCCESS) {
            printf("%s", nl);
            free(kvd);
            return ec;
        }
        printf(",%s", nl);
        PRINT_INDENT(0, "\"keyValueData\":%s{%s", space, nl)
        printKVDataJSON(kvd, pHeader->keyValueData.byteLength, base_indent + 1, indent_width, minified);
        free(kvd);
        PRINT_INDENT_NOARG(0, "}")
    }

    if (hasSGD) {
        printf(",%s", nl);
        PRINT_INDENT(0, "\"supercompressionGlobalData\":%s{%s", space, nl)

        switch (pHeader->supercompressionScheme) {
        case KTX_SS_NONE: {
            PRINT_INDENT(1, "\"type\":%s\"%s\"%s", space, "KTX_SS_NONE", nl)
            break;
        }
        case KTX_SS_BASIS_LZ: {
            PRINT_INDENT(1, "\"type\":%s\"%s\"", space, "KTX_SS_BASIS_LZ")
            ktx_size_t sgdByteLength = pHeader->supercompressionGlobalData.byteLength;
            ktx_uint8_t* sgd = malloc(sgdByteLength);
            if (sgd == NULL)
                return KTX_OUT_OF_MEMORY;
            ec = stream->setpos(stream, pHeader->supercompressionGlobalData.byteOffset);
            if (ec != KTX_SUCCESS) {
                printf("%s", nl);
                PRINT_INDENT(0, "}%s", nl)
                free(sgd);
                return ec;
            }
            ec = stream->read(stream, sgd, sgdByteLength);
            if (ec != KTX_SUCCESS) {
                printf("%s", nl);
                PRINT_INDENT(0, "}%s", nl)
                free(sgd);
                return ec;
            }

            // Calculate number of images
            uint32_t layersFaces = MAX(pHeader->layerCount, 1) * pHeader->faceCount;
            uint32_t layerPixelDepth = MAX(pHeader->pixelDepth, 1);
            for (uint32_t level = 1; level < MAX(pHeader->levelCount, 1); level++)
                layerPixelDepth += MAX(MAX(pHeader->pixelDepth, 1) >> level, 1U);
            // NOTA BENE: faceCount * layerPixelDepth is only reasonable because
            // faceCount and depth can't both be > 1. I.e there are no 3d cubemaps.
            uint32_t numImages = layersFaces * layerPixelDepth;
            ktxBasisLzGlobalHeader* bgdh = (ktxBasisLzGlobalHeader*)(sgd);

            if (sgdByteLength < sizeof(ktxBasisLzGlobalHeader)) {
                printf("%s", nl);
                PRINT_INDENT(0, "}%s", nl)
                free(sgd);
                return ec;
            }
            printf(",%s", nl);
            PRINT_INDENT(1, "\"endpointCount\":%s%u,%s", space, bgdh->endpointCount, nl)
            PRINT_INDENT(1, "\"selectorCount\":%s%u,%s", space, bgdh->selectorCount, nl)
            PRINT_INDENT(1, "\"endpointsByteLength\":%s%u,%s", space, bgdh->endpointsByteLength, nl)
            PRINT_INDENT(1, "\"selectorsByteLength\":%s%u,%s", space, bgdh->selectorsByteLength, nl)
            PRINT_INDENT(1, "\"tablesByteLength\":%s%u,%s", space, bgdh->tablesByteLength, nl)
            PRINT_INDENT(1, "\"extendedByteLength\":%s%u,%s", space, bgdh->extendedByteLength, nl)
            PRINT_INDENT(1, "\"images\":%s[", space)

            ktxBasisLzEtc1sImageDesc* slices = (ktxBasisLzEtc1sImageDesc*)(sgd + sizeof(ktxBasisLzGlobalHeader));
            for (ktx_uint32_t i = 0; i < numImages; i++) {
                if (sgdByteLength < (i + 1) * sizeof(ktxBasisLzEtc1sImageDesc) + sizeof(ktxBasisLzGlobalHeader))
                    break;

                if (i == 0)
                    printf("%s", nl);
                else
                    printf(",%s", nl);

                PRINT_INDENT(2, "{%s", nl)

                buFlags imageFlags = slices[i].imageFlags;
                if (imageFlags == 0) {
                    PRINT_INDENT(3, "\"imageFlags\":%s[],%s", space, nl)
                } else {
                    PRINT_INDENT(3, "\"imageFlags\":%s[%s", space, nl)
                    printFlagBitsJSON(LENGTH_OF_INDENT(4), nl, imageFlags, ktxBUImageFlagsBitString);
                    PRINT_INDENT(3, "],%s", nl)
                }

                PRINT_INDENT(3, "\"rgbSliceByteLength\":%s%u,%s", space, slices[i].rgbSliceByteLength, nl)
                PRINT_INDENT(3, "\"rgbSliceByteOffset\":%s%u,%s", space, slices[i].rgbSliceByteOffset, nl)
                PRINT_INDENT(3, "\"alphaSliceByteLength\":%s%u,%s", space, slices[i].alphaSliceByteLength, nl)
                PRINT_INDENT(3, "\"alphaSliceByteOffset\":%s%u%s", space, slices[i].alphaSliceByteOffset, nl)
                PRINT_INDENT_NOARG(2, "}")
            }
            printf("%s", nl);
            PRINT_INDENT(1, "]%s", nl)

            free(sgd);
            break;
        }
        case KTX_SS_ZSTD: {
            PRINT_INDENT(1, "\"type\":%s\"%s\"%s", space, "KTX_SS_ZSTD", nl)
            break;
        }
        case KTX_SS_ZLIB: {
            PRINT_INDENT(1, "\"type\":%s\"%s\"%s", space, "KTX_SS_ZLIB", nl)
            break;
        }
        default:
            PRINT_INDENT(1, "\"type\":%s%u%s", space, pHeader->supercompressionScheme, nl)
            break;
        }
        PRINT_INDENT_NOARG(0, "}")
    }
    printf("%s", nl);

    return ec;
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
            if (result != KTX_SUCCESS)
                return result;
            result = printKTX2Info2(stream, &header.ktx2);
        }
    }
    return result;
}

/**
 * @internal
 * @~English
 * @brief Print information about a KTX2 file.
 *
 * The stream's read pointer should be at the start of the file.
 *
 * @param [in]  stream  pointer to the ktxStream reading the file.
 */
KTX_error_code
ktxPrintKTX2InfoJSONForStream(ktxStream* stream, ktx_uint32_t base_indent, ktx_uint32_t indent_width, bool minified)
{
    ktx_uint8_t ktx2_ident_ref[12] = KTX2_IDENTIFIER_REF;
    KTX_header2 header;
    KTX_error_code result;

    assert(stream != NULL);

    result = stream->read(stream, &header, sizeof(ktx2_ident_ref));
    if (result != KTX_SUCCESS)
        return result;

    // Compare identifier, is this a KTX2 file?
    if (memcmp(header.identifier, ktx2_ident_ref, 12))
        return KTX_UNKNOWN_FILE_FORMAT;

    // Read rest of header.
    result = stream->read(stream, &header.vkFormat, KTX2_HEADER_SIZE - sizeof(ktx2_ident_ref));
    if (result != KTX_SUCCESS)
        return result;

    result = printKTX2Info2JSON(stream, &header, base_indent, indent_width, minified);
    return result;
}

/**
 * @internal
 * @~English
 * @brief Print information about a KTX2 file.
 *
 * The stream's read pointer should be at the start of the file.
 *
 * @param [in]  stream  pointer to the ktxStream reading the file.
 */
KTX_error_code
ktxPrintKTX2InfoTextForStream(ktxStream* stream)
{
    ktx_uint8_t ktx2_ident_ref[12] = KTX2_IDENTIFIER_REF;
    KTX_header2 header;
    KTX_error_code result;

    assert(stream != NULL);

    result = stream->read(stream, &header, sizeof(ktx2_ident_ref));
    if (result != KTX_SUCCESS)
        return result;

    // Compare identifier, is this a KTX2 file?
    if (memcmp(header.identifier, ktx2_ident_ref, 12))
        return KTX_UNKNOWN_FILE_FORMAT;

    // Read rest of header.
    result = stream->read(stream, &header.vkFormat, KTX2_HEADER_SIZE - sizeof(ktx2_ident_ref));
    if (result != KTX_SUCCESS)
        return result;

    result = printKTX2Info2(stream, &header);
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
    // TODO: Implement
    UNUSED(filename);
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
}

/**
 * @~English
 * @brief Print information about a KTX2 file on a stdioStream in JSON format.
 *
 * The stdioStream's read pointer should be at the start of the file.
 *
 * @param [in]  stream  pointer to the ktxStream reading the file.
 * @param [in]  base_indent The number of indentations to include at the front of every line.
 * @param [in]  indent_width The number of spaces to add with each nested scope.
 * @param [in]  minified Specifies whether the JSON output should be minified.
 */
KTX_error_code
ktxPrintKTX2InfoJSONForStdioStream(FILE* stdioStream, ktx_uint32_t base_indent, ktx_uint32_t indent_width, bool minified)
{
    KTX_error_code result;
    ktxStream stream;

    if (stdioStream == NULL)
        return KTX_INVALID_VALUE;

    result = ktxFileStream_construct(&stream, stdioStream, KTX_FALSE);
    if (result == KTX_SUCCESS)
        result = ktxPrintKTX2InfoJSONForStream(&stream, base_indent, indent_width, minified);
    return result;
}

/**
 * @~English
 * @brief Print information about a KTX2 file in JSON format.
 *
 * @param [in]  filename     Filepath of the KTX2 file.
 * @param [in]  base_indent  The number of indentations to include at the front of every line.
 * @param [in]  indent_width The number of spaces to add with each nested scope.
 * @param [in]  minified     Specifies whether the JSON output should be minified.
 */
KTX_error_code
ktxPrintKTX2InfoJSONForNamedFile(const char* const filename, ktx_uint32_t base_indent, ktx_uint32_t indent_width, bool minified)
{
    FILE* file = NULL;

    file = ktxFOpenUTF8(filename, "rb");

    if (!file)
        return KTX_FILE_OPEN_FAILED;

    KTX_error_code result = ktxPrintKTX2InfoJSONForStdioStream(file, base_indent, indent_width, minified);

    fclose(file);
    return result;
}

/**
 * @~English
 * @brief Print information about a KTX2 file in memory in JSON format.
 *
 * @param [in]  bytes   pointer to the memory holding the KTX file.
 * @param [in]  size    length of the KTX file in bytes.
 * @param [in]  base_indent The number of indentations to include at the front of every line.
 * @param [in]  indent_width The number of spaces to add with each nested scope.
 * @param [in]  minified Specifies whether the JSON output should be minified.
 */
KTX_error_code
ktxPrintKTX2InfoJSONForMemory(const ktx_uint8_t* bytes, ktx_size_t size, ktx_uint32_t base_indent, ktx_uint32_t indent_width, bool minified)
{
    KTX_error_code result;
    ktxStream stream;

    result = ktxMemStream_construct_ro(&stream, bytes, size);
    if (result == KTX_SUCCESS)
        result = ktxPrintKTX2InfoJSONForStream(&stream, base_indent, indent_width, minified);
    return result;
}

/**
 * @~English
 * @brief Print information about a KTX2 file on a stdioStream in textual format.
 *
 * The stdioStream's read pointer should be at the start of the file.
 *
 * @param [in]  stream  pointer to the ktxStream reading the file.
 */
KTX_error_code
ktxPrintKTX2InfoTextForStdioStream(FILE* stdioStream)
{
    KTX_error_code result;
    ktxStream stream;

    if (stdioStream == NULL)
        return KTX_INVALID_VALUE;

    result = ktxFileStream_construct(&stream, stdioStream, KTX_FALSE);
    if (result == KTX_SUCCESS)
        result = ktxPrintKTX2InfoTextForStream(&stream);
    return result;
}

/**
 * @~English
 * @brief Print information about a KTX2 file in JSON format.
 *
 * @param [in]  filename     Filepath of the KTX2 file.
 */
KTX_error_code
ktxPrintKTX2InfoTextForNamedFile(const char* const filename)
{
    FILE* file = NULL;

#ifdef _WIN32
    fopen_s(&file, filename, "rb");
#else
    file = fopen(filename, "rb");
#endif

    if (!file)
        return KTX_FILE_OPEN_FAILED;

    KTX_error_code result = ktxPrintKTX2InfoTextForStdioStream(file);

    fclose(file);
    return result;
}

/**
 * @~English
 * @brief Print information about a KTX2 file in memory in textual format.
 *
 * @param [in]  bytes   pointer to the memory holding the KTX file.
 * @param [in]  size    length of the KTX file in bytes.
 */
KTX_error_code
ktxPrintKTX2InfoTextForMemory(const ktx_uint8_t* bytes, ktx_size_t size)
{
    KTX_error_code result;
    ktxStream stream;

    result = ktxMemStream_construct_ro(&stream, bytes, size);
    if (result == KTX_SUCCESS)
        result = ktxPrintKTX2InfoTextForStream(&stream);
    return result;
}
