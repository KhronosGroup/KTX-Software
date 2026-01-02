#!/bin/bash

GOOD=astcenc-good.exe
BAD=astcenc-avx2.exe
BLOCK_SIZE=4x4
QUALITY=-fastest

./bin/${GOOD} -tl in.png out-good.png ${BLOCK_SIZE} ${QUALITY} -silent

./bin/${BAD} -tl in.png out-bad.png ${BLOCK_SIZE} ${QUALITY} -silent

compare out-good.png out-bad.png out-vs-out-diff.png
compare in.png out-bad.png out-vs-in-diff.png

./bin/astc_test_autoextract ${BLOCK_SIZE} in.png out-good.png out-bad.png in-min.png

./bin/${GOOD} -tl in-min.png out-min-good.png ${BLOCK_SIZE} ${QUALITY} -silent

./bin/${BAD} -tl in-min.png out-min-bad.png ${BLOCK_SIZE} ${QUALITY} -silent

compare out-min-good.png out-min-bad.png out-min-diff.png
