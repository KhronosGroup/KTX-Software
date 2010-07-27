/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file	sample.h
 * @brief	List of the samples to be used.
 *
 * $Revision$
 * $Date::                            $
 *
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

#ifndef __SAMPLE_H__
#define __SAMPLE_H__

/* ----------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------- */

#include "at.h"

/* ----------------------------------------------------------------------------- */
/* SAMPLE 01 */
void atInitialize_01_draw_texture(void** ppAppData);
void atRelease_01_draw_texture(void* pAppData);
void atResize_01_draw_texture(void* pAppData, int iWidth, int iHeight);
void atRun_01_draw_texture(void* pAppData, int iTimeMS); 

static const atSample sc_Sample01 = {
	atInitialize_01_draw_texture,
	atRelease_01_draw_texture,
	atResize_01_draw_texture,
	atRun_01_draw_texture,
};

/* ----------------------------------------------------------------------------- */
/* SAMPLE 02 */
void atInitialize_02_cube(void** ppAppData);
void atRelease_02_cube(void* pAppData);
void atResize_02_cube(void* pAppData, int iWidth, int iHeight);
void atRun_02_cube(void* pAppData, int iTimeMS); 

static const atSample sc_Sample02 = {
	atInitialize_02_cube,
	atRelease_02_cube,
	atResize_02_cube,
	atRun_02_cube,
};

#if 0
/* ----------------------------------------------------------------------------- */
/* SAMPLE 03 */
void atInitialize_03_teapot(void** ppAppData);
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
void atInitialize_04_bunny(void** ppAppData);
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

static const atSample* const sc_aSamples[] = {
	&sc_Sample01,
	&sc_Sample02,
	/* &sc_Sample03,
	&sc_Sample04, */

};

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

/* ----------------------------------------------------------------------------- */

#endif /*__SAMPLE_H__*/

