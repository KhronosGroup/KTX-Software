#!/bin/bash

if [ -z "$1" ]; then
    echo "ERROR: Missing root; e.g. for ref-main-avx2 set 'main'"
    exit 1
fi

echo "Generating new ref-$1 results"

if [ "$1" = "main" ]; then
    echo "Using binary from ./bin/${1}/"
else
    echo "Using binary from ./Binaries/${1}/"
fi

echo ""

TARGET_ROOT=${1}

python3 ./Test/astc_test_image.py --test-set all --block-size all --test-quality all --repeats 6 --encoder ref-$1-avx2
#python3 ./Test/astc_test_image.py --test-set all --block-size all --test-quality all --repeats 6 --encoder ref-$1-sse4.1
#python3 ./Test/astc_test_image.py --test-set all --block-size all --test-quality all --repeats 6 --encoder ref-$1-sse2
