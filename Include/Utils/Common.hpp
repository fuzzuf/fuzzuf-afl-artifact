#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <cassert>
#include <tuple>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <functional>
#include <exception>
#include <set>
#include <string>
#include <vector>
#include <queue>
#include <system_error>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <Utils/Status.hpp>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
#ifdef __x86_64__
typedef unsigned long long u64;
#else
typedef uint64_t u64;
#endif /* ^__x86_64__ */

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

#define SWAP16(_x) ({ \
    u16 _ret = (_x); \
    (u16)((_ret << 8) | (_ret >> 8)); \
    })

#define SWAP32(_x) ({ \
    u32 _ret = (_x); \
    (u32)((_ret << 24) | (_ret >> 24) | \
        ((_ret << 8) & 0x00FF0000) | \
        ((_ret >> 8) & 0x0000FF00)); \
    })
    
#define likely(_x)   __builtin_expect(!!(_x), 1)
#define unlikely(_x)  __builtin_expect(!!(_x), 0)

#define MEM_BARRIER() \
  __asm__ volatile("" ::: "memory")

// Instead of T*, we should use T& and this in usual cases because T* is ambiguous 
// in the point that we can't see whether the pointer refers to array or an element
// also raw pointers are relatively dangerous 
template<class T>
using NullableRef = std::optional<std::reference_wrapper<T>>;

class InvalidOption : public std::invalid_argument {
public:
    InvalidOption(const std::string& what_arg) : std::invalid_argument(what_arg) {}
};

class FileError : public std::invalid_argument {
public:
    FileError(const std::string& what_arg) : std::invalid_argument(what_arg) {}
};

namespace Util {

/*
namespace {
    template<typename T>
    struct func_trait;

    template<typename R, typename... P>
    struct func_trait<R(P...)> {
        using return_type_t = R;
        using args_type_t = P;
    };
};

template<class T>
using GetReturnType = typename func_trait<T>::return_type_t;

template<class T>
using GetFuncArgsType = typename func_trait<T>::args_type_t;
*/

void CreateDir(std::string path);    

int OpenFile(std::string path, int flag);
int OpenFile(std::string path, int flag, mode_t mode);

void ReadFile(int fd, void *buf, u32 len);
u32 ReadFileTimed(int fd, void *buf, u32 len, u32 timeout_ms);

void WriteFile(int fd, const void *buf, u32 len);
void WriteFileStr(int fd, std::string str);

int FSync(int fd);

off_t SeekFile(int fd, off_t offset, int whence);

int TruncateFile(int fd, off_t length);

void CopyFile(std::string from, std::string to);
void CloseFile(int fd);

void DeleteFileOrDirectory(std::string path);

int ScanDirAlpha(std::string dir, struct dirent ***namelist);

int GetCpuCore();
std::set<int> GetFreeCpu(int);

u64 NextP2(u64);

pid_t Fork();

u64 GetCurTimeUs();
u64 GetCurTimeMs();

u32 Hash32(const void* key, u32 len, u32 seed);

u32 CountBits( const u8* mem, u32 len);
u32 CountBytes( const u8* mem, u32 len);
u32 CountNon255Bytes( const u8* mem, u32 len);

void MinimizeBits(u8* dst, const u8* src, u32 len);

std::tuple< s32, s32 > LocateDiffs( const u8* ptr1, const u8* ptr2, u32 len );

std::string StrPrintf(const char *format, ...);

u64 GlobalCounter();

class set_segv_handler {
public:
  set_segv_handler( const set_segv_handler& ) = delete;
  set_segv_handler( set_segv_handler&& ) = delete;
  set_segv_handler &operator-( const set_segv_handler& ) = delete;
  set_segv_handler &operator=( set_segv_handler&& ) = delete;
  static const set_segv_handler &get();
private:
  set_segv_handler();
};
/**
 * @fn
 * この関数の呼び出し以降、log()に渡されたログがurlで指定されたfluentdに送られる
 * ローカルホストの24224番ポートで動くfluentdに接続するには"fluent://localhost:24224"を設定する
 * 接続先のポートが24224番(デフォルト)の場合はポート番号は省略できる
 * init_loggerを再度呼び出す事でログの送信先を変更できるが、変更した時点で既に送信準備に入っていたログがinit_loggerの呼び出し後に古い送信先に送られる可能性がある
 * init_logger時点では実際の接続は行われない為、指定された送信先に実際にログを送れるかはチェックされない
 * ログを受け取るfluentdはurlに指定したアドレスでforward inputを待ち受けている必要がある
 * @brief ログの送信先のfluentdのURLを設定する
 * @param url 接続先のfluentdのURL
 **/
void init_logger( const std::string &url );
/**
 * @fn
 * log()に渡されたログがfluentdに送られる状態になっているかを確認する
 * この関数は送信先が設定されていることだけをチェックする為、実際にfluentdがログを受け取れる状態になっているかはこの関数の結果に影響しない
 * @brief ログの送信先が設定されているかを確認する
 * @return 送信先のfluentdが設定されていればtrue、そうでなければfalseが返る
 **/
bool has_logger();
/**
 * @fn
 * init_loggerで設定された送信先に文字列のログを送る
 * 文字列はそれがJSONとして解釈可能な場合JSONとしてパースしてfluentdに送られる
 * JSONとして解釈できない文字列の場合、その文字列だけを含むJSONとしてfluentdに送られる
 * ログのタグはwyvern.<親プロセスのPID>.<自身のPID>.<tagで指定した値>になる
 * この関数はログの送信を送信キューに積んで送信完了を待たずに返る
 * @brief ログを送信する
 * @param tag このログのタグを指定する
 * @param message ログ
 * @param cb ログの送信が完了または失敗した際に呼ばれるコールバック
 **/
void log( std::string &&tag, std::string &&message, std::function< void( fuzzuf::status_t ) > &&cb );
/**
 * @fn
 * init_loggerで設定された送信先に文字列のログを送る
 * 文字列はそれがJSONとして解釈可能な場合JSONとしてパースしてfluentdに送られる
 * JSONとして解釈できない文字列の場合、その文字列だけを含むJSONとしてfluentdに送られる
 * ログのタグはwyvern.<親プロセスのPID>.<自身のPID>.<tagで指定した値>になる
 * この関数はログの送信が完了するか失敗するまでブロックする
 * @brief ログを送信する
 * @param tag このログのタグを指定する
 * @param sync 同期送信を行うかどうかを指定する
 * @param message ログ
 * @return ログの送信の結果
 **/
fuzzuf::status_t log( std::string &&tag, std::string &&message );
/**
 * @fn
 * init_loggerで設定された送信先にJSONのログを送る
 * ログのタグはwyvern.<親プロセスのPID>.<自身のPID>.<tagで指定した値>になる
 * この関数はログの送信を送信キューに積んで送信完了を待たずに返る
 * @brief ログを送信する
 * @param tag このログのタグを指定する
 * @param message ログ
 * @param cb ログの送信が完了または失敗した際に呼ばれるコールバック
 **/
void log( std::string &&tag, nlohmann::json &&message, std::function< void( fuzzuf::status_t ) > &&cb );
/**
 * @fn
 * init_loggerで設定された送信先にJSONのログを送る
 * ログのタグはwyvern.<親プロセスのPID>.<自身のPID>.<tagで指定した値>になる
 * この関数はログの送信が完了するか失敗するまでブロックする
 * @brief ログを送信する
 * @param tag このログのタグを指定する
 * @param sync 同期送信を行うかどうかを指定する
 * @param message ログ
 * @return ログの送信の結果
 **/
fuzzuf::status_t log( std::string &&tag, nlohmann::json &&message );

};

#endif
