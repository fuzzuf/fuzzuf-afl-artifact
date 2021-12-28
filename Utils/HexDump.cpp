#include <stdio.h>
#include <assert.h>
#include "Utils/HexDump.hpp"

// ファイルポインタ fp で指定されたストリームに、バッファ buf のオフセット offset から len バイトだけHex Dumpしてくれる便利関数
// offset は16の倍数にしてください。でないとi+jのせいで壊れます
// FIXME: 暗黙のLinux前提
void HexDump(FILE* fp, unsigned char* buf, size_t len, size_t offset) {
    assert(buf);
    fprintf(fp, "(size = %#lx)\n", len);
    size_t end = offset + len;
    for (size_t i = offset; i < end; i += 0x10) {
        fprintf(fp, "%08lx: ", i / 0x10 * 0x10);
        for (int j = 0; j < 0x10 && i + j < end; j += 1){
            fprintf(fp, "%02x ", buf[i + j]);
            if (j == 7) fprintf(fp, "   ");
        }
        fprintf(fp, "\n");
    }
}
