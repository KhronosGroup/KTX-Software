/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2010-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file unittests.cc
 * @~English
 *
 * @brief Tests of internal API functions.
 *
 * @author Mark Callow, Edgewise Consulting
 */

#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
#endif

#include "gtest/gtest.h"
#include "image.hpp"

namespace {

////////////////////////
// normalisation tests
////////////////////////

template<typename ColorType>
void test_color(ColorType input, ColorType output) {
    ColorType color;
    for (uint32_t i = 0; i < color.comps_count(); ++i) {
        color.set(i, static_cast<typename ColorType::value_type>(input[i]));
    }

    // Verify color components were set correctly
    for (uint32_t i = 0; i < color.comps_count(); ++i) {
        EXPECT_EQ(color[i], static_cast<typename ColorType::value_type>(input[i]));
    }

    color.normalize();

    for (uint32_t  i = 0; i < color.comps_count(); ++i) {
        EXPECT_EQ(color[i], static_cast<typename ColorType::value_type>(output[i]));
    }
}

static constexpr uint32_t min_val{0};
static constexpr uint32_t max_val[3]={255, 65535, 4294967295};
static constexpr uint32_t some_val[3]={31, 191, 178};

template<uint32_t componentCount>
void test_by_channel( const uint32_t min_res[], const uint32_t max_res[],
    const uint32_t some_res8[], const uint32_t some_res16[], const uint32_t some_res32[]) {
    {
        color<uint8_t, componentCount> c8i{(uint8_t)min_val, (uint8_t)min_val, (uint8_t)min_val, (uint8_t)min_val};
        color<uint8_t, componentCount> c8o{(uint8_t)min_res[0], (uint8_t)min_res[0], (uint8_t)min_res[0], 0};
        test_color(c8i, c8o);
    }
    {
        color<uint8_t, componentCount> c8i{(uint8_t)max_val[0], (uint8_t)max_val[0], (uint8_t)max_val[0], (uint8_t)max_val[0]};
        color<uint8_t, componentCount> c8o{(uint8_t)max_res[0], (uint8_t)max_res[0], (uint8_t)max_res[0], (uint8_t)max_val[0]};
        test_color(c8i, c8o);
    }
    {
        color<uint8_t, componentCount> c8i{(uint8_t)some_val[0], (uint8_t)some_val[1], (uint8_t)some_val[2], 0};
        color<uint8_t, componentCount> c8o{(uint8_t)some_res8[0], (uint8_t)some_res8[1], (uint8_t)some_res8[2], 0};
        test_color(c8i, c8o);
    }
    {
        color<uint16_t, componentCount> c16i{(uint16_t)min_val, (uint16_t)min_val, (uint16_t)min_val, (uint16_t)min_val};
        color<uint16_t, componentCount> c16o{(uint16_t)min_res[1], (uint16_t)min_res[1], (uint16_t)min_res[1], 0};
        test_color(c16i, c16o);
    }
    {
        color<uint16_t, componentCount> c16i{(uint16_t)max_val[1], (uint16_t)max_val[1], (uint16_t)max_val[1], (uint16_t)max_val[1]};
        color<uint16_t, componentCount> c16o{(uint16_t)max_res[1], (uint16_t)max_res[1], (uint16_t)max_res[1], (uint16_t)max_val[1]};
        test_color(c16i, c16o);
    }
    {
        color<uint16_t, componentCount> c16i{(uint16_t)some_val[0], (uint16_t)some_val[1], (uint16_t)some_val[2], 0};
        color<uint16_t, componentCount> c16o{(uint16_t)some_res16[0], (uint16_t)some_res16[1], (uint16_t)some_res16[2], 0};
        test_color(c16i, c16o);
    }
    {
        color<uint32_t, componentCount> c32i{(uint32_t)min_val, (uint32_t)min_val, (uint32_t)min_val, (uint32_t)min_val};
        color<uint32_t, componentCount> c32o{(uint32_t)min_res[2], (uint32_t)min_res[2], (uint32_t)min_res[2], 0};
        test_color(c32i, c32o);
    }
    {
        color<uint32_t, componentCount> c32i{(uint32_t)max_val[2], (uint32_t)max_val[2], (uint32_t)max_val[2], (uint32_t)max_val[2]};
        color<uint32_t, componentCount> c32o{(uint32_t)max_res[2], (uint32_t)max_res[2], (uint32_t)max_res[2], (uint32_t)max_val[2]};
        test_color(c32i, c32o);
    }
    {
        color<uint32_t, componentCount> c32i{(uint32_t)some_val[0], (uint32_t)some_val[1], (uint32_t)some_val[2], 0};
        color<uint32_t, componentCount> c32o{(uint32_t)some_res32[0], (uint32_t)some_res32[1], (uint32_t)some_res32[2], 0};
        test_color(c32i, c32o);
    }
}

// Hand tested vectors and their normalised results in GeoGebra
static constexpr uint32_t min_res[3]={54, 13849, 907633408};     // These are -0.577350 float in 8bit-UNORM, 16bit-UNORM and 32bit-UNORM values
static constexpr uint32_t max_res[3]={201, 51686, 3387333888};

static constexpr uint32_t some_res8[3]={30, 192, 179};
static constexpr uint32_t some_res16[3]={13790, 13883, 13875};
static constexpr uint32_t some_res32[3]={907633408, 907633408, 907633408};

static constexpr uint32_t min_res2[3]={37, 9597, 628983424};
static constexpr uint32_t max_res2[3]={218, 55938, 3665984000};
static constexpr uint32_t some_res8_2[3]={21, 198, 128};
static constexpr uint32_t some_res16_2[3]={9541, 9654, 32768};
static constexpr uint32_t some_res32_2[3]={628983424, 628983424, 2147483648};

static constexpr uint32_t min_res1[3]={0, 0, 0};
static constexpr uint32_t max_res1[3]={255, 65535, 4294967295};
static constexpr uint32_t some_res8_1[3]={255, 0, 0};            // Second and third values are unused because its single channel image result
static constexpr uint32_t some_res16_1[3]={65535, 0, 0};         // Same as above
static constexpr uint32_t some_res32_1[3]={4294967295, 0, 0};    // Same as above

TEST(NormaliseColorTest, multi_channel_test) {
    test_by_channel<4>(min_res, max_res, some_res8, some_res16, some_res32);
    test_by_channel<3>(min_res, max_res, some_res8, some_res16, some_res32);
    test_by_channel<2>(min_res2, max_res2, some_res8_2, some_res16_2, some_res32_2);
    test_by_channel<1>(min_res1, max_res1, some_res8_1, some_res16_1, some_res32_1);
}

}  // namespace
