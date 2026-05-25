#!/bin/bash

# Copyright 2026 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

echo "creating BC1 KTX2 file from RGB8 UNORM PNG input ..."
ktx create --testrun --encode uastc --format R8G8B8_UNORM \
  --assign-tf linear --assign-primaries bt709 \
  ./tests/resources/input/png/rgb8_unorm_bc1.png - | \
  ktx transcode --testrun --target bc1 - ./tests/resources/ktx2/rgb8_unorm_bc1.ktx2

echo "creating BC1 KTX2 file from RGB8 SRGB PNG input ..."
ktx create --testrun --encode uastc --format R8G8B8_SRGB \
  --assign-tf srgb --assign-primaries bt709 \
  ./tests/resources/input/png/rgb8_srgb_bc1.png - | \
  ktx transcode --testrun --target bc1 - ./tests/resources/ktx2/rgb8_srgb_bc1.ktx2

echo "creating BC3 KTX2 file from RGBA8 UNORM PNG input ..."
ktx create --testrun --encode uastc --format R8G8B8A8_UNORM \
  --assign-tf linear --assign-primaries bt709 \
  ./tests/resources/input/png/rgba8_unorm_bc3.png - | \
  ktx transcode --testrun --target bc3 - ./tests/resources/ktx2/rgba8_unorm_bc3.ktx2

echo "creating BC3 KTX2 file from RGBA8 SRGB PNG input ..."
ktx create --testrun --encode uastc --format R8G8B8A8_SRGB \
  --assign-tf srgb --assign-primaries bt709 \
  ./tests/resources/input/png/rgba8_srgb_bc3.png - | \
  ktx transcode --testrun --target bc3 - ./tests/resources/ktx2/rgba8_srgb_bc3.ktx2

echo "creating BC4 KTX2 file from R8 UNORM PNG input ..."
ktx create --testrun --encode uastc --format R8_UNORM \
  --assign-tf linear --assign-primaries bt709 \
  ./tests/resources/input/png/r8_unorm_bc4.png - | \
  ktx transcode --testrun --target bc4 - ./tests/resources/ktx2/r8_unorm_bc4.ktx2

echo "creating BC5 KTX2 file from RG8 UNORM PNG input ..."
ktx create --testrun --encode uastc --format R8G8_UNORM \
  --assign-tf linear --assign-primaries bt709 \
  ./tests/resources/input/png/rg8_unorm_bc5.png - | \
  ktx transcode --testrun --target bc5 - ./tests/resources/ktx2/rg8_unorm_bc5.ktx2

# EXR input file can be generated from input PNG file via OIIO:
# oiiotool tests/resources/input/png/rgb16_sfloat_bc6hu.png --iscolorspace srgb --tocolorspace lin_rec709_srgb -d half -o tests/resources/input/exr/rgb16_sfloat_bc6hu.exr
echo "creating BC6HU KTX2 file from RGB16 EXR input ..."
ktx create --testrun --encode uastc-hdr-4x4 --format R16G16B16_SFLOAT \
  ./tests/resources/input/exr/rgb16_sfloat_bc6hu.exr - | \
  ktx transcode --testrun --target bc6hu - ./tests/resources/ktx2/rgb16_sfloat_bc6hu.ktx2

echo "creating BC7 KTX2 file from RGBA8 UNORM PNG input ..."
ktx create --testrun --encode uastc --format R8G8B8A8_UNORM \
  --assign-tf linear --assign-primaries bt709 \
  ./tests/resources/input/png/rgba8_unorm_bc7.png - | \
  ktx transcode --testrun --target bc7 - ./tests/resources/ktx2/rgba8_unorm_bc7.ktx2

echo "creating BC7 KTX2 file from RGBA8 SRGB PNG input ..."
ktx create --testrun --encode uastc --format R8G8B8A8_SRGB \
  --assign-tf srgb --assign-primaries bt709 \
  ./tests/resources/input/png/rgba8_srgb_bc7.png - | \
  ktx transcode --testrun --target bc7 - ./tests/resources/ktx2/rgba8_srgb_bc7.ktx2
