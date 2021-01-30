/*
 * Copyright (c) 2020, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "decompress_sources.h"
#include <linux/zstd.h>

#define CONTROL(x)                                                             \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "%s:%u: %s failed!\n", __FUNCTION__, __LINE__, #x);      \
      abort();                                                                 \
    }                                                                          \
  } while (0)


static const char kEmptyZstdFrame[] = {
    0x28, 0xb5, 0x2f, 0xfd, 0x24, 0x00, 0x01, 0x00, 0x00, 0x99, 0xe9, 0xd8, 0x51
};

static void test_decompress_unzstd() {
    fprintf(stderr, "Testing decompress unzstd... ");
    {
        size_t const wkspSize = ZSTD_estimateDCtxSize();
        void* wksp = malloc(wkspSize);
        CONTROL(wksp != NULL);
        ZSTD_DCtx* dctx = ZSTD_initStaticDCtx(wksp, wkspSize);
        CONTROL(dctx != NULL);
        size_t const dSize = ZSTD_decompressDCtx(dctx, NULL, 0, kEmptyZstdFrame, sizeof(kEmptyZstdFrame));
        CONTROL(!ZSTD_isError(dSize));
        CONTROL(dSize == 0);
        free(wksp);
    }
    fprintf(stderr, "Ok\n");
}

int main(void) {
  test_decompress_unzstd();
  return 0;
}
