/*
 * Copyright 2016-2020 Mark Callow
 * SPDX-License-Identifier: Apache-2.0
 */

static const float cube_face[] =
{
    -1.0f, +1.0f, +1.0f, /* Front */
    +1.0f, -1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    -1.0f, -1.0f, +1.0f,
    -1.0f, +1.0f, -1.0f, /* Back */
    +1.0f, -1.0f, -1.0f,
    +1.0f, +1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    +1.0f, -1.0f, +1.0f, /* Right */
    +1.0f, +1.0f, -1.0f,
    +1.0f, +1.0f, +1.0f,
    +1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, +1.0f, /* Left */
    -1.0f, +1.0f, -1.0f,
    -1.0f, +1.0f, +1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, +1.0f, /* Bottom */
    +1.0f, -1.0f, -1.0f,
    +1.0f, -1.0f, +1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, +1.0f, +1.0f, /* Top */
    +1.0f, +1.0f, -1.0f,
    +1.0f, +1.0f, +1.0f,
    -1.0f, +1.0f, -1.0f,
};
#define CUBE_NUM_FACE_COMPONENTS 3
#define CUBE_FACE_STRIDE (sizeof(float) * CUBE_NUM_FACE_COMPONENTS)

static const float cube_color[] = /* almost random colors */
{
    0.7f, 0.1f, 0.2f, 0.0f,     0.8f, 0.9f, 0.3f, 0.0f,     0.4f, 1.0f, 0.5f, 0.0f,     0.0f, 0.6f, 0.1f, 0.0f,
    0.8f, 0.2f, 0.3f, 0.0f,     0.9f, 1.0f, 0.4f, 0.0f,     0.5f, 0.0f, 0.6f, 0.0f,     0.1f, 0.7f, 0.2f, 0.0f,
    0.9f, 0.3f, 0.4f, 0.0f,     1.0f, 0.0f, 0.5f, 0.0f,     0.6f, 0.1f, 0.7f, 0.0f,     0.2f, 0.8f, 0.3f, 0.0f,
    1.0f, 0.4f, 0.5f, 0.0f,     0.0f, 0.1f, 0.6f, 0.0f,     0.7f, 0.2f, 0.8f, 0.0f,     0.3f, 0.9f, 0.4f, 0.0f,
    0.0f, 0.5f, 0.6f, 0.0f,     0.1f, 0.2f, 0.7f, 0.0f,     0.8f, 0.3f, 0.9f, 0.0f,     0.4f, 1.0f, 0.5f, 0.0f,
    0.1f, 0.6f, 0.7f, 0.0f,     0.2f, 0.3f, 0.8f, 0.0f,     0.9f, 0.4f, 1.0f, 0.0f,     0.5f, 0.0f, 0.6f, 0.0f,
};
static const float cube_texture[] =
{
    0.0f, 1.0f,     1.0f, 0.0f,     1.0f, 1.0f,     0.0f, 0.0f,
    1.0f, 1.0f,     0.0f, 0.0f,     0.0f, 1.0f,     1.0f, 0.0f,
    0.0f, 1.0f,     1.0f, 0.0f,     1.0f, 1.0f,     0.0f, 0.0f,
    1.0f, 1.0f,     0.0f, 0.0f,     0.0f, 1.0f,     1.0f, 0.0f,
    0.0f, 1.0f,     1.0f, 0.0f,     1.0f, 1.0f,     0.0f, 0.0f,
    1.0f, 1.0f,     0.0f, 0.0f,     0.0f, 1.0f,     1.0f, 0.0f
};
static const float cube_normal[] =
{
    0.0f, 0.0f, +1.0f,  0.0f, 0.0f, +1.0f,  0.0f, 0.0f, +1.0f,  0.0f, 0.0f, +1.0f,
    0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,
    +1.0f, 0.0f, 0.0f,  +1.0f, 0.0f, 0.0f,  +1.0f, 0.0f, 0.0f,  +1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
    0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,
    0.0f, +1.0f, 0.0f,  0.0f, +1.0f, 0.0f,  0.0f, +1.0f, 0.0f,  0.0f, +1.0f, 0.0f,
};

static const unsigned short cube_index_buffer[] = {
     0, 3, 1, 2, 0, 1,  /* Front */
     6, 5, 4, 5, 7, 4,  /* Back */
     8,11, 9,10, 8, 9,  /* Right */
    15,12,13,12,14,13,  /* Left */
    16,19,17,18,16,17,  /* Bottom */
    23,20,21,20,22,21   /* Top */
};
#define CUBE_NUM_INDICES (sizeof(cube_index_buffer) / sizeof(unsigned short))
