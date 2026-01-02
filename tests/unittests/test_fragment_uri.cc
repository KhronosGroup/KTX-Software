/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */
// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include <ktx/fragment_uri.h>
#include "gtest/gtest.h"


namespace {

// -------------------------------------------------------------------------------------------------

class FragmentURITest : public ::testing::Test {
protected:
    FragmentURITest() {}
};

TEST_F(FragmentURITest, ParseWholeRange) {
    EXPECT_EQ(ktx::parseFragmentURI("").mip, ktx::SelectorRange{});
    EXPECT_EQ(ktx::parseFragmentURI("").facial, ktx::SelectorRange{});
    EXPECT_EQ(ktx::parseFragmentURI("").stratal, ktx::SelectorRange{});

    EXPECT_EQ(ktx::parseFragmentURI("m").mip, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("m").stratal, ktx::SelectorRange{});
    EXPECT_EQ(ktx::parseFragmentURI("m").facial, ktx::SelectorRange{});
    EXPECT_EQ(ktx::parseFragmentURI("a").mip, ktx::SelectorRange{});
    EXPECT_EQ(ktx::parseFragmentURI("a").stratal, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("a").facial, ktx::SelectorRange{});
    EXPECT_EQ(ktx::parseFragmentURI("f").mip, ktx::SelectorRange{});
    EXPECT_EQ(ktx::parseFragmentURI("f").stratal, ktx::SelectorRange{});
    EXPECT_EQ(ktx::parseFragmentURI("f").facial, ktx::SelectorRange(ktx::all));

    EXPECT_EQ(ktx::parseFragmentURI("m&a").mip, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("m&a").stratal, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("a&f").stratal, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("a&f").facial, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("f&m").mip, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("f&m").facial, ktx::SelectorRange(ktx::all));
}

TEST_F(FragmentURITest, ParseRangeEmpty) {
    EXPECT_EQ(ktx::parseFragmentURI("m=").mip, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("a=").stratal, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("f=").facial, ktx::SelectorRange(ktx::all));

    EXPECT_EQ(ktx::parseFragmentURI("m=,").mip, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("a=,").stratal, ktx::SelectorRange(ktx::all));
    EXPECT_EQ(ktx::parseFragmentURI("f=,").facial, ktx::SelectorRange(ktx::all));
}

TEST_F(FragmentURITest, ParseRangeBegin) {
    EXPECT_EQ(ktx::parseFragmentURI("m=0").mip, ktx::SelectorRange(0, ktx::RangeEnd));
    EXPECT_EQ(ktx::parseFragmentURI("a=0").stratal, ktx::SelectorRange(0, ktx::RangeEnd));
    EXPECT_EQ(ktx::parseFragmentURI("f=0").facial, ktx::SelectorRange(0, ktx::RangeEnd));

    EXPECT_EQ(ktx::parseFragmentURI("m=1").mip, ktx::SelectorRange(1, ktx::RangeEnd));
    EXPECT_EQ(ktx::parseFragmentURI("a=1").stratal, ktx::SelectorRange(1, ktx::RangeEnd));
    EXPECT_EQ(ktx::parseFragmentURI("f=1").facial, ktx::SelectorRange(1, ktx::RangeEnd));
}

TEST_F(FragmentURITest, ParseRangeEnd) {
    EXPECT_EQ(ktx::parseFragmentURI("m=,0").mip, ktx::SelectorRange(0, 1));
    EXPECT_EQ(ktx::parseFragmentURI("a=,0").stratal, ktx::SelectorRange(0, 1));
    EXPECT_EQ(ktx::parseFragmentURI("f=,0").facial, ktx::SelectorRange(0, 1));

    EXPECT_EQ(ktx::parseFragmentURI("m=,1").mip, ktx::SelectorRange(0, 2));
    EXPECT_EQ(ktx::parseFragmentURI("a=,1").stratal, ktx::SelectorRange(0, 2));
    EXPECT_EQ(ktx::parseFragmentURI("f=,1").facial, ktx::SelectorRange(0, 2));
}

TEST_F(FragmentURITest, ParseRangeBeginEnd) {
    EXPECT_EQ(ktx::parseFragmentURI("m=0,0").mip, ktx::SelectorRange(0, 1));
    EXPECT_EQ(ktx::parseFragmentURI("a=0,0").stratal, ktx::SelectorRange(0, 1));
    EXPECT_EQ(ktx::parseFragmentURI("f=0,0").facial, ktx::SelectorRange(0, 1));

    EXPECT_EQ(ktx::parseFragmentURI("m=0,1").mip, ktx::SelectorRange(0, 2));
    EXPECT_EQ(ktx::parseFragmentURI("a=0,1").stratal, ktx::SelectorRange(0, 2));
    EXPECT_EQ(ktx::parseFragmentURI("f=0,1").facial, ktx::SelectorRange(0, 2));

    EXPECT_EQ(ktx::parseFragmentURI("m=1,3").mip, ktx::SelectorRange(1, 4));
    EXPECT_EQ(ktx::parseFragmentURI("a=1,3").stratal, ktx::SelectorRange(1, 4));
    EXPECT_EQ(ktx::parseFragmentURI("f=1,3").facial, ktx::SelectorRange(1, 4));
}

TEST_F(FragmentURITest, ParseMultipleRange) {
    EXPECT_EQ(ktx::parseFragmentURI("m=0,0&a=1,1").mip, ktx::SelectorRange(0, 1));
    EXPECT_EQ(ktx::parseFragmentURI("m=0,0&a=1,1").stratal, ktx::SelectorRange(1, 2));
    EXPECT_EQ(ktx::parseFragmentURI("a=0,0&f=1,1").stratal, ktx::SelectorRange(0, 1));
    EXPECT_EQ(ktx::parseFragmentURI("a=0,0&f=1,1").facial, ktx::SelectorRange(1, 2));
    EXPECT_EQ(ktx::parseFragmentURI("f=0,0&m=1,1").facial, ktx::SelectorRange(0, 1));
    EXPECT_EQ(ktx::parseFragmentURI("f=0,0&m=1,1").mip, ktx::SelectorRange(1, 2));
}

TEST_F(FragmentURITest, ParseMultiRange) {
    EXPECT_EQ(fmt::format("{}", ktx::parseFragmentURI("m=10,15&m=20,").mip), "10..15,20..last");
    EXPECT_EQ(fmt::format("{}", ktx::parseFragmentURI("m=0,0&m=1,1&m=10,15&m=20,").mip), "0,1,10..15,20..last");
}

TEST_F(FragmentURITest, Validate) {
    EXPECT_TRUE(ktx::parseFragmentURI("m=0,0&a=1,1").validate(1, 2, 1));
    EXPECT_FALSE(ktx::parseFragmentURI("m=0,0&a=1,1").validate(1, 1, 1));
    EXPECT_FALSE(ktx::parseFragmentURI("m=0,0&a=1,1").validate(0, 0, 0));
}

TEST_F(FragmentURITest, SelectorRangeToString) {
    EXPECT_EQ(fmt::format("{}", ktx::SelectorRange{0, 0}), "none");
    EXPECT_EQ(fmt::format("{}", ktx::SelectorRange{0, 1}), "0");
    EXPECT_EQ(fmt::format("{}", ktx::SelectorRange{10, 11}), "10");
    EXPECT_EQ(fmt::format("{}", ktx::SelectorRange{0, 2}), "0..1");
    EXPECT_EQ(fmt::format("{}", ktx::SelectorRange{10, 12}), "10..11");
    EXPECT_EQ(fmt::format("{}", ktx::SelectorRange{0, ktx::RangeEnd}), "all");
}

}  // namespace
