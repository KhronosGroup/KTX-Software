#!/bin/bash

BLOCK_SIZE=6x6
QUALITY=-thorough

./astcenc/astcenc-avx2-good -tl in.png out-good.png ${BLOCK_SIZE} ${QUALITY} -silent

./astcenc/astcenc-avx2 -tl in.png out-bad.png ${BLOCK_SIZE} ${QUALITY} -silent

compare out-good.png out-bad.png out-vs-out-diff.png
compare in.png out-bad.png out-vs-in-diff.png

./astcenc/astc_test_autoextract ${BLOCK_SIZE} in.png out-good.png out-bad.png in-min.png

./astcenc/astcenc-avx2-good -tl in-min.png out-min-good.png ${BLOCK_SIZE} ${QUALITY} -silent

./astcenc/astcenc-avx2 -tl in-min.png out-min-bad.png ${BLOCK_SIZE} ${QUALITY} -silent

compare out-min-good.png out-min-bad.png out-min-diff.png
