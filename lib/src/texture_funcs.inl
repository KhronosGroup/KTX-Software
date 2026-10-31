/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2019-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Templates for functions common to base & derived ktxTexture classes.
 *
 * Define CLASS before including this file.
 */

#define CAT(c, n) PRIMITIVE_CAT(c, n)
#define PRIMITIVE_CAT(c, n) c ## _ ## n

#define CLASS_FUNC(name) CAT(CLASS, name)

/*
 ======================================
     Virtual ktxTexture functions
 ======================================
*/


KTX_error_code CLASS_FUNC(GLUpload)(CLASS* This, GLuint* pTexture,
                                    GLenum* pTarget, GLenum* pGlerror);

/*
 ======================================
     Internal ktxTexture functions
 ======================================
*/


void CLASS_FUNC(destruct)(CLASS* This);

