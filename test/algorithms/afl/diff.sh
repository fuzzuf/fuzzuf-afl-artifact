#!/bin/bash

set -eu
set -o pipefail

cd `dirname $0`

cd ./original-afl-outdir
rm -f ../original-afl-diff
for i in $( find queue/ -maxdepth 1 | grep id | sort ); do
    md5sum $i >> ../original-afl-diff
done
for i in $( find queue/.state/auto_extras/ -maxdepth 1 -type f | sort ); do
    md5sum $i >> ../original-afl-diff
done
find queue/.state/deterministic_done/ -maxdepth 1 | sort >> ../original-afl-diff 
find queue/.state/redundant_edges/    -maxdepth 1 | sort >> ../original-afl-diff 
find queue/.state/variable_behavior/  -maxdepth 1 | sort >> ../original-afl-diff 

cd ../fuzzuf-afl-outdir
rm -f ../fuzzuf-afl-diff
for i in $( find queue/ -maxdepth 1 | grep id | sort ); do
    md5sum $i >> ../fuzzuf-afl-diff
done
for i in $( find queue/.state/auto_extras/ -maxdepth 1 -type f | sort ); do
    md5sum $i >> ../fuzzuf-afl-diff
done
find queue/.state/deterministic_done/ -maxdepth 1 | sort >> ../fuzzuf-afl-diff 
find queue/.state/redundant_edges/    -maxdepth 1 | sort >> ../fuzzuf-afl-diff 
find queue/.state/variable_behavior/  -maxdepth 1 | sort >> ../fuzzuf-afl-diff 

cd ../
diff original-afl-diff fuzzuf-afl-diff
