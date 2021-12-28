#!/bin/bash

set -eu
set -o pipefail

cd `dirname $0`

mkdir ./original-afl-outdir
patch -N -d AFL/ -p1 < original-afl.patch
cd AFL
make
./afl-fuzz -t 3000 -i ../../../put_binaries/libjpeg/seeds -o ../original-afl-outdir -- ../../../put_binaries/libjpeg/libjpeg_turbo_fuzzer @@ | tee original-afl-stdout

cd ../
mkdir ./fuzzuf-afl-outdir
../../../fuzzuf afl --in_dir=../../put_binaries/libjpeg/seeds --out_dir=./fuzzuf-afl-outdir -- ../../put_binaries/libjpeg/libjpeg_turbo_fuzzer @@ | tee fuzzuf-afl-stdout
