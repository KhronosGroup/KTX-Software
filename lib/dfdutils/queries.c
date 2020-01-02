/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright (c) 2019 The Khronos Group Inc.
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
 * @file
 * @~English
 * @brief Utilities for querying info from a data format descriptor.
 * @author Mark Callow
 */

#include <stdint.h>
#include <stdio.h>
#include <KHR/khr_df.h>
#include "dfd.h"

/**
 * @~English
 * @brief Get the number and size of the image components from a DFD.
 *
 * This simplified function is for use only with the DFDs for unpacked
 * formats which means all components have the same size.
 *
 * @param DFD Pointer to a Data Format Descriptor to interpret,
              described as 32-bit words in native endianness.
              Note that this is the whole descriptor, not just
              the basic descriptor block.
 * @param numComponents pointer to a 32-bit word in which the number of
                        components will be written.
 * @param componentByteLength pointer to a 32-bit word in which the size of
                              a component in bytes will be written.
 */
void
getDFDComponentInfoUnpacked(const uint32_t* DFD, uint32_t* numComponents,
                            uint32_t* componentByteLength)
{
    const uint32_t *BDFDB = DFD+1;
    uint32_t numSamples = KHR_DFDSAMPLECOUNT(BDFDB);
    uint32_t sampleCounter;
    uint32_t currentChannel = ~0U; /* Don't start matched. */

    /* This is specifically for unpacked formats which means the size of */
    /* each component is the same. */
    *numComponents = 0;
    for (sampleCounter = 0; sampleCounter < numSamples; ++sampleCounter) {
        uint32_t sampleByteLength = (KHR_DFDSVAL(BDFDB, sampleCounter, BITLENGTH) + 1) >> 3U;
        uint32_t sampleChannel = KHR_DFDSVAL(BDFDB, sampleCounter, CHANNELID);

        if (sampleChannel == currentChannel) {
            /* Continuation of the same channel. */
            /* Accumulate the byte length. */
            *componentByteLength += sampleByteLength;
        } else {
            /* Everything is new. Hopefully. */
            currentChannel = sampleChannel;
            (*numComponents)++;
            *componentByteLength = sampleByteLength;
        }
    }
}

uint32_t getDFDNumComponents(const uint32_t* DFD)
{
    const uint32_t *BDFDB = DFD+1;
    uint32_t currentChannel = ~0U; /* Don't start matched. */
    uint32_t numComponents = 0;
    uint32_t numSamples = KHR_DFDSAMPLECOUNT(BDFDB);
    uint32_t sampleCounter;

    for (sampleCounter = 0; sampleCounter < numSamples; ++sampleCounter) {
        uint32_t sampleChannel = KHR_DFDSVAL(BDFDB, sampleCounter, CHANNELID);
        if (sampleChannel != currentChannel) {
            numComponents++;
            currentChannel = sampleChannel;
        }
    }
    return numComponents;
}
