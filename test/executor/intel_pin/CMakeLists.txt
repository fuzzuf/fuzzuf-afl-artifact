### pinbinとpintoolsをソースディレクトリからビルドディレクトリへコピーする。

file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/pin-3.7-linux
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
)

file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/pintools
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
)

### FIXME: pintoolの1つであるbbcounts2.soが必要とする外部出力ファイルをビルドディレクトリへコピーする
file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/crash.bin
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../
)
file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/image.offset
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../
)