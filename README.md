# FUZZUF

## Requirements
* CMake
    * Recommend to use latest version

## How to build
```shell
### リポジトリのルートディレクトリで実行
cmake -B build -DDEFAULT_RUNLEVEL=Debug
cmake --build build
```

## How to test
CIで実行しているテストは以下と同等です。

```shell
cmake --build build --target test ARGS=-V
```

### How to run
#### CLI tool
```shell
rm -rf /tmp/out_dir run.log
build/fuzzuf afl --in_dir=test/put_binaries/libjpeg/seeds/ --out_dir=/tmp/out_dir --log_file=run.log test/put_binaries/libjpeg/libjpeg_turbo_fuzzer @@
```
