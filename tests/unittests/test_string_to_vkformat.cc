/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */
// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "vkformat_enum.h"
#include "gtest/gtest.h"


extern "C" {
    VkFormat stringToVkFormat(const char* str);
}

// -------------------------------------------------------------------------------------------------

class StringToVkFormatTest : public ::testing::Test {
protected:
    StringToVkFormatTest() {}
};

// -------------------------------------------------------------------------------------------------

namespace {

TEST_F(StringToVkFormatTest, stringToVkFormat) {
    EXPECT_EQ(stringToVkFormat("UNDEFINED"), VK_FORMAT_UNDEFINED);
    EXPECT_EQ(stringToVkFormat("VK_FORMAT_UNDEFINED"), VK_FORMAT_UNDEFINED);
    EXPECT_EQ(stringToVkFormat("Not a format"), VK_FORMAT_UNDEFINED);

    EXPECT_EQ(stringToVkFormat("R4G4_UNORM_PACK8"), VK_FORMAT_R4G4_UNORM_PACK8);
    EXPECT_EQ(stringToVkFormat("VK_FORMAT_R4G4_UNORM_PACK8"), VK_FORMAT_R4G4_UNORM_PACK8);

    EXPECT_EQ(stringToVkFormat("R8G8B8_UNORM"), VK_FORMAT_R8G8B8_UNORM);
    EXPECT_EQ(stringToVkFormat("VK_FORMAT_R8G8B8_UNORM"), VK_FORMAT_R8G8B8_UNORM);
    EXPECT_EQ(stringToVkFormat("R8G8B8_SNORM"), VK_FORMAT_R8G8B8_SNORM);
    EXPECT_EQ(stringToVkFormat("VK_FORMAT_R8G8B8_SNORM"), VK_FORMAT_R8G8B8_SNORM);

    EXPECT_EQ(stringToVkFormat("ASTC_6x6_UNORM_BLOCK"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    EXPECT_EQ(stringToVkFormat("ASTC_6X6_UNORM_BLOCK"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    EXPECT_EQ(stringToVkFormat("astc_6x6_unorm_block"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    EXPECT_EQ(stringToVkFormat("VK_FORMAT_ASTC_6x6_UNORM_BLOCK"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    EXPECT_EQ(stringToVkFormat("VK_FORMAT_ASTC_6X6_UNORM_BLOCK"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    EXPECT_EQ(stringToVkFormat("VK_FORMAT_ASTC_6X6_UNORM_BLOCK"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    EXPECT_EQ(stringToVkFormat("vk_format_astc_6x6_unorm_block"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK);

    EXPECT_EQ(stringToVkFormat("VK_FORMAT_ASTC_6x6_UNORM"), VK_FORMAT_UNDEFINED);
    EXPECT_EQ(stringToVkFormat("VK_FORMAT_ASTC_6x6_UNORM_BLOC"), VK_FORMAT_UNDEFINED);
    EXPECT_EQ(stringToVkFormat("K_FORMAT_ASTC_6x6_UNORM_BLOCK"), VK_FORMAT_UNDEFINED);
    EXPECT_EQ(stringToVkFormat("_ASTC_6x6_UNORM_BLOCK"), VK_FORMAT_UNDEFINED);
    EXPECT_EQ(stringToVkFormat("STC_6x6_UNORM_BLOCK"), VK_FORMAT_UNDEFINED);
}

} // namespace
