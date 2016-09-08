/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: 9bc124bf5480416f4c8155c2cb8c871fb84fce17 $ */

/**
 * @file	sample.h
 * @brief	List of the samples to be used.
 */

/*
 * Copyright (c) 2008 HI Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of HI Corporation."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */


#include "../common/LoadTests.h"

extern "C" {
/* ----------------------------------------------------------------------------- */
/* SAMPLE 01 */
void atInitialize_01_draw_texture(void** ppAppData, const char* const szArgs,
                                  const char* const szBasePath);
void atRelease_01_draw_texture(void* pAppData);
void atResize_01_draw_texture(void* pAppData, int iWidth, int iHeight);
void atRun_01_draw_texture(void* pAppData, tTicks tTicks); 

static const atSample sc_Sample01 = {
	atInitialize_01_draw_texture,
	atRelease_01_draw_texture,
	atResize_01_draw_texture,
	atRun_01_draw_texture,
};

/* ----------------------------------------------------------------------------- */
/* SAMPLE 02 */
void atInitialize_02_cube(void** ppAppData, const char* const szArgs,
                          const char* const szBasePath);
void atRelease_02_cube(void* pAppData);
void atResize_02_cube(void* pAppData, int iWidth, int iHeight);
void atRun_02_cube(void* pAppData, tTicks tTicks); 

static const atSample sc_Sample02 = {
	atInitialize_02_cube,
	atRelease_02_cube,
	atResize_02_cube,
	atRun_02_cube,
};
  
}

#if 0
/* ----------------------------------------------------------------------------- */
/* SAMPLE 03 */
void atInitialize_03_teapot(void** ppAppData, const char* const* args);
void atRelease_03_teapot(void* pAppData);
void atResize_03_teapot(int iWidth, int iHeight);
void atRun_03_teapot(void* pAppData, int iTimeMS); 

static const atSample sc_Sample03 = {
	atInitialize_03_teapot,
	atRelease_03_teapot,
	atResize_03_teapot,
	atRun_03_teapot,
};

/* ----------------------------------------------------------------------------- */
/* SAMPLE 04 */
void atInitialize_04_bunny(void** ppAppData, const char* const* args);
void atRelease_04_bunny(void* pAppData);
void atResize_04_bunny(int iWidth, int iHeight);
void atRun_04_bunny(void* pAppData, int iTimeMS); 

static const atSample sc_Sample04 = {
	atInitialize_04_bunny,
	atRelease_04_bunny,
	atResize_04_bunny,
	atRun_04_bunny,
};
#endif

/* ----------------------------------------------------------------------------- */

const LoadTests::sampleInvocation siSamples[] = {
	{ &sc_Sample01, "--npot testimages/hi_mark.ktx", "RGB8 NPOT HI Logo" },
	{ &sc_Sample01, "--npot testimages/luminance_unsized_reference.ktx", "Luminance (Unsized) NPOT" },
	{ &sc_Sample01, "testimages/up-reference.ktx", "RGB8" },
	{ &sc_Sample01, "testimages/down-reference.ktx", "RGB8 + KTXOrientation" },
	{ &sc_Sample01, "testimages/etc1.ktx", "ETC1 RGB8"},
	{ &sc_Sample01, "testimages/etc2-rgb.ktx", "ETC2 RGB8"},
	{ &sc_Sample01, "testimages/etc2-rgba1.ktx", "ETC2 RGB8A1" },
	{ &sc_Sample01, "testimages/etc2-rgba8.ktx", "ETC2 RGB8A8" },
	{ &sc_Sample01, "testimages/rgba-reference.ktx", "RGBA8" },
	{ &sc_Sample02, "testimages/rgb-reference.ktx", "RGB8" },
	{ &sc_Sample02, "testimages/rgb-amg-reference.ktx", "RGB8 + Auto Mipmap" },
	{ &sc_Sample02, "testimages/rgb-mipmap-reference.ktx", "Color/level mipmap" },
	{ &sc_Sample02, "--npot testimages/hi_mark_sq.ktx", "RGB8 NPOT HI Logo" }
};

const int iNumSamples = sizeof(siSamples) / sizeof(LoadTests::sampleInvocation);


AppBaseSDL* theApp = new LoadTests(siSamples, iNumSamples,
                                   "KTX Loader Tests for OpenGL ES 1");


