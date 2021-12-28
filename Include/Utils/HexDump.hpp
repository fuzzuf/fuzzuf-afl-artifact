#pragma once

// FIXME: 暗黙のLinux前提
void HexDump(FILE* fp, unsigned char* buf, size_t len, size_t offset);