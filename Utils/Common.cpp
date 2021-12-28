#include "config.h"
#include <memory>
#include <thread>
#include <cassert>
#include <numeric>
#include <future>
#ifdef HAS_CXX_STD_BIT
#include <bit>
#endif
#ifdef HAS_CXX_STD_FILESYSTEM
#include <filesystem>
namespace filesystem = std::filesystem;
#else
#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;
#endif
#include <boost/asio.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/stacktrace.hpp>
#include <nlohmann/json.hpp>
#include <config.h>
#ifdef FLC_FOUND
#include <flc/flc.hpp>
#endif
#include <signal.h>
#include <execinfo.h>
#include "Utils/Common.hpp"
#include "Logger/Logger.hpp"


/* Maximum line length passed from GCC to 'as' and used for parsing
   configuration files: */

#define MAX_LINE            8192

// TODO: Linuxでの実行を想定した実装になっている。
//  - 名前空間を `LinuxUtil` へリネーム
//  - 型チェックにより変なデータが入らないことを保証
//      - 例： `ReadFile(LinuxFileDescriptor fd, void *buf, u32 len)`
//  - パスとして無効な string を投げられないように、引数の型を std::filesystem::path にしていいとおもう。

// FIXME: エラークラス名は FileError じゃなくて IoError or IOError が適切では？

namespace Util {

// TODO: ディレクトリを作成したら true を、作成する必要が無かったら false を返すようにしていいのでは？セマンティクスが不一致。
void CreateDir(std::string path) {
    if (filesystem::exists(path) && filesystem::is_directory(path)) {
        // Do nothing
        return;
    }
    if (mkdir(path.c_str(), 0700)) {
        throw FileError("Unable to create directory: " + path);
    }
}

int OpenFile(std::string path, int flag) {
    int fd = open(path.c_str(), flag);
    if (fd < 0) 
        throw FileError("Unable to open file: " + path);
    return fd;
}

int OpenFile(std::string path, int flag, mode_t mode) {
    int fd = open(path.c_str(), flag, mode);
    if (fd < 0) 
        throw FileError("Unable to open file: " + path);
    return fd;    
}

/*
    since read/write sometimes reads/writes less bytes than specified by the 3rd argument 'n',
    it's safe to wrap read/write so that they should read/write just 'n' bytes
 */
static ssize_t read_n(int fd, void *buf, size_t n) {
  size_t nread = 0;
  while (nread < n) {
      ssize_t res = read(fd, (char*)buf + nread, n - nread);
      if (res == -1) {
        // moreover, some of errors indicate we should retry
        // see man page
        if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
          continue;
        }
        return -1;
      }
      if (res == 0) {
          break;
      }
      nread += res;
  }
  return nread;
}

static ssize_t write_n(int fd, const void *buf, size_t n) {
  size_t nwritten = 0;
  while (nwritten < n) {
      ssize_t res = write(fd, (char*)buf + nwritten, n - nwritten);
      if (res == -1) {
         if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
            continue;
         }
         return -1;
      }
      nwritten += res;
  }
  return nwritten;
}

void ReadFile(int fd, void *buf, u32 len) {
    if (read_n(fd, buf, len) != len)
      throw FileError(StrPrintf("Failed to read fd=%d", fd));
}

void WriteFile(int fd, const void *buf, u32 len) {
    if (write_n(fd, buf, len) != len)
      throw FileError(StrPrintf("Failed to write fd=%d", fd));
}

// 時間制限付きReadFile
// https://github.com/AFLplusplus/AFLplusplus/blob/stable/src/afl-forkserver.c#L135-L210
// timeout_ms ミリ秒後にタイムアウトする
// 戻り値は、エラー発生時に0、タイムアウト時にtimeout_ms+1
// それ以外の時はかかった時間(ミリ秒)
// fdがinvalid等のエラーに加えて、fdからlenバイト読めない場合もエラーになるので注意
// タイマーはselectシステムコールによって実現している
// select, readそれぞれがsignalのinterruptなどにより、失敗することを考慮に入れないといけない
// あるいは、readは指定したバイト数lenだけ一度に読み込むとは限らない
// したがって、ループによって複数回処理する必要がある
// 複数回実行される時の注意点として、残り時間の更新がある。
// Linuxにおいてはselectが引数timeoutの値を書き換え、「select中に経過した時間をtimeoutの値から差し引く処理」が実行される。すなわち自動で残り時間が更新されていく。
// それ以外の環境においては書き換えられないことが知られているため、時間を計測し、自前で残りの時間を計算する必要がある
// ただしこれらはドキュメントで保証されている挙動ではないように見えるため、どう対応するかは今後検討しなければならない

u32 ReadFileTimed(int fd, void *buf, u32 len, u32 timeout_ms) {
    ssize_t nread = 0;
    struct timeval timeout;

    timeout.tv_sec = (timeout_ms / 1000);
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

#if !defined(__linux__)
    u64 read_start = GetCurTimeMs();
#endif

    while (true) {
        // loop内でselectを使うときは毎回初期化が必要(select man参照)
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        // selectでtimeout時間だけ待つ
        int sret = select(fd + 1, &readfds, NULL, NULL, &timeout);

        // 戻り値が正、0、負で場合分け
        if (likely(sret > 0)) { // 戻り値が正の時、fdから読み出し準備ができているはず
            // 読み出し処理。ただしreadがシグナル割り込みなどで失敗する可能性があるため、do whileで囲む
            do {
                ssize_t rret = read(fd, (char*)buf + nread, len - nread);

                // selectが消費した時間の計測と残り時間の更新
#if defined(__linux__)
                u32 exec_ms = timeout_ms - (timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#else
                u32 exec_ms = (u32)std::min((u64)timeout_ms, GetCurTimeMs() - read_start);
                u32 remain_ms = timeout_ms - exec_ms;
                timeout.tv_sec = (remain_ms / 1000);
                timeout.tv_usec = (remain_ms % 1000) * 1000;
#endif

                if (likely(rret) > 0) { // fdからバイトが読めている、ただし lenバイト読めているかは分からない
                    nread += rret; // 今まで読んだバイト数を合算
                    if (likely(nread == len)) { // 一致していたらreturnしてよい
                      return std::max<u32>(exec_ms, 1); // 0を返すとエラーになってしまうので1で切り上げる
                    }
                    break; // 一致していない場合はまだreadがいる。ただしselectに戻るのでbreak
                } else if (unlikely(rret == 0)) { // 0が返ってきたらファイルの末端に来ているということ。lenバイト読めないので失敗
                    return 0;
                } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) { // この時点でrretは-1確定。先に完全なエラーのみ処理
                    return 0;
                }

                // 割り込み等でreadをやり直したいが、すでに時間超過していたらタイムアウト
                if (exec_ms >= timeout_ms) {
                    return timeout_ms + 1;
                }

                // 何事もなければやり直し
                // ここに来るのはつまりEINTR, EAGAIN, EWOULDBLOCKなどが発生し
                // かつ時間超過していない場合
            } while(1);
        } else if (unlikely(sret == 0)) { // タイムアウトしてselectがreturnしたことを意味する
            return timeout_ms + 1;
        } 

        // sret < 0を意味する。EINTRだけはシグナル割り込みによる失敗なので再度トライ
        // FIXME: 非Linux環境でtimeoutを修正しないことによりbusy loopになるレアケースがあるか（ないと思うし時間計測頻繁にやると重いのであまり入れたくない）
        // 現状timeoutの修正をreadのloop内でやっているため、外側だけをloopされるとbusy loopになる恐れはなくはない
        if (likely(errno == EINTR)) continue;

        // その他何かしらの失敗なのでエラー
        return 0;
    }
}

void WriteFileStr(int fd, std::string str) {
    WriteFile(fd, (void *)str.c_str(), str.size());
}

int FSync(int fd) {
    return fsync(fd);
}

off_t SeekFile(int fd, off_t offset, int whence) {
    off_t result = 0;
    if ( ( result = lseek(fd, offset, whence) ) < 0)
        throw FileError(StrPrintf(
            "Failed to lseek(fd=%d, offset=%d, whence=%d) = %d",
            fd, offset, whence, result
            ));
    return result;
}

// 前提：
//  - fd は有効なファイルディスクリプタであり、ファイルであること
// 責務：
//  - fd で指定されたファイルを length で指定された長さに切り詰める
int TruncateFile(int fd, off_t length) {
    int result = 0u;
    if ( ( result = ftruncate(fd, length) ) < 0)
        throw FileError(StrPrintf("Failed to truncate fd=%d", fd));
    return result;
}

void CopyFile(std::string from, std::string to) {
    std::ifstream  src(from, std::ios::binary);
    std::ofstream  dst(to,   std::ios::binary);
    dst << src.rdbuf();
}

// 責務：
//  - fd で指定されたファイルディスクリプタを閉じる
void CloseFile(int fd) {
    close(fd);
}

void DeleteFileOrDirectory(std::string path) {
  int res = remove(path.c_str());
  if (res) {
    DEBUG("Warning: %s could not be deleted.\n", path.c_str());
  }
}

int ScanDirAlpha(std::string dir, struct dirent ***namelist) {
    return scandir(dir.c_str(), namelist, NULL, alphasort);
}

#include "Algorithms/AFL/AFLMacro.hpp"
int GetCpuCore() {
    int cpu_core_count = 0;
  #if defined(__APPLE__) || defined(__FreeBSD__) || defined (__OpenBSD__)
    size_t s = sizeof(cpu_core_count);
    /* On *BSD systems, we can just use a sysctl to get the number of CPUs. */
  #ifdef __APPLE__
    if (sysctlbyname("hw.logicalcpu", &cpu_core_count, &s, NULL, 0) < 0)
        return;
  #else
    int s_name[2] = { CTL_HW, HW_NCPU };
    if (sysctl(s_name, 2, &cpu_core_count, &s, NULL, 0) < 0) return;
  #endif /* ^__APPLE__ */

  #else
  #ifdef HAVE_AFFINITY
    cpu_core_count = sysconf(_SC_NPROCESSORS_ONLN);
  #else
    FILE* f = fopen("/proc/stat", "r");
    u8 tmp[1024];

    if (!f) return;

    while (fgets(tmp, sizeof(tmp), f))
      if (!strncmp(tmp, "cpu", 3) && isdigit(tmp[3])) cpu_core_count++;

    fclose(f);

  #endif /* ^HAVE_AFFINITY */

  #endif /* ^(__APPLE__ || __FreeBSD__ || __OpenBSD__) */
  return cpu_core_count;    
}

std::set<int> GetFreeCpu(int allcpu) {
    std::set<int> used, frees;
    struct dirent* de;
    DIR* d = opendir("/proc");
    if (!d) {
        ERROR("Unable to access /proc - can't scan for free CPU cores.");
    }
 
    // FIXME: maybe this should be a random value because this sleep is inserted
    // in order to avoid lots of processes doing this procedure at the same time
    usleep(500 * 250);
    
    /* Scan all /proc/<pid>/status entries, checking for Cpus_allowed_list.
         Flag all processes bound to a specific CPU using cpu_used[]. This will
         fail for some exotic binding setups, but is likely good enough in almost
         all real-world use cases. */
    while ((de = readdir(d))) {
        std::string fn;
        FILE* f;
        char tmp[MAX_LINE];
        bool has_vmsize = false;

        if (!isdigit(de->d_name[0])) continue;

        fn = StrPrintf("/proc/%s/status", de->d_name);

        if (!(f = fopen(fn.c_str(), "r"))) {
            continue;
        }

        while (fgets(tmp, MAX_LINE, f)) {
            int hval;          
            /* Processes without VmSize are probably kernel tasks. */
            if (!strncmp(tmp, "VmSize:\t", 8)) has_vmsize = true;

            if (!strncmp(tmp, "Cpus_allowed_list:\t", 19) &&
                !strchr(tmp, '-') && !strchr(tmp, ',') &&
                sscanf(tmp + 19, "%d", &hval) == 1 && has_vmsize) {
                used.insert(hval);
                break;
            }
        }
    }
    closedir(d);

    for (int i = 0; i < allcpu; i++) {
        if (used.find(i) == used.end())
            frees.insert(i);
    }
    return frees;
}

u64 NextP2(u64 val) {
    u64 ret = 1;
    while (val > ret) ret <<= 1;
    return ret;
}

pid_t Fork() {
    return fork();
}

u64 GetCurTimeMs() {
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    return (tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000);    
}

u64 GetCurTimeUs() {
    struct timeval tv;
    struct timezone tz;
    
    gettimeofday(&tv, &tz);
    return (tv.tv_sec * 1000000ULL) + tv.tv_usec;
}

std::string StrPrintf(const char *format, ...) { 
    va_list va, va2;
    va_start(va, format);
    va_copy(va2, va);
    
    const int _len = vsnprintf (nullptr, 0, format, va);
    //if (_len < 0) ABORT("Whoa, vsnprintf() fails?!");
    try {
        std::string res(_len, ' ');
        std::vsnprintf( &res.front(), _len+1, format, va2) ;
        va_end(va2);
        va_end(va);
        return res;
    } catch(const std::bad_alloc&) {
        va_end(va2);
        va_end(va);
        throw;
    }    
}

#ifdef __x86_64__

#define ROL64(_x, _r)  ((((u64)(_x)) << (_r)) | (((u64)(_x)) >> (64 - (_r))))

u32 Hash32(const void* key, u32 len, u32 seed) {
    const u64* data = (u64*)key;
    u64 h1 = seed ^ len;

    len >>= 3;

    while (len--) {
        u64 k1 = *data++;

        k1 *= 0x87c37b91114253d5ULL;
        k1  = ROL64(k1, 31);
        k1 *= 0x4cf5ad432745937fULL;

        h1 ^= k1;
        h1  = ROL64(h1, 27);
        h1  = h1 * 5 + 0x52dce729;
    }
    
    h1 ^= h1 >> 33;
    h1 *= 0xff51afd7ed558ccdULL;
    h1 ^= h1 >> 33;
    h1 *= 0xc4ceb9fe1a85ec53ULL;
    h1 ^= h1 >> 33;

    return h1;    
}
#else 

#define ROL32(_x, _r)  ((((u32)(_x)) << (_r)) | (((u32)(_x)) >> (32 - (_r))))

u32 Hash32(const void* key, u32 len, u32 seed) {
    const u32* data  = (u32*)key;
    u32 h1 = seed ^ len;
    
    len >>= 2;

    while (len--) {

        u32 k1 = *data++;

        k1 *= 0xcc9e2d51;
        k1  = ROL32(k1, 15);
        k1 *= 0x1b873593;

        h1 ^= k1;
        h1  = ROL32(h1, 13);
        h1  = h1 * 5 + 0xe6546b64;
    }
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    
    return h1;
}

#endif /* ^__x86_64__ */

/* Count the number of bits set in the provided bitmap. Used for the status
   screen several times every second, does not have to be fast. */

u32 CountBits( const u8* mem, u32 len) {
    assert( len % sizeof( u32 ) == 0 );
    const u32* ptr = reinterpret_cast< const u32* >( mem );
#ifdef __cpp_lib_bitops
    return std::accumulate(
      ptr, std::next( ptr, len >> 2 ), u32( 0 ),
      []( u32 sum, u32 v ){ return sum + std::popcount( v ); }
    );
#else
    u32  i   = (len >> 2);
    u32  ret = 0;

    while (i--) {

        u32 v = *(ptr++);

        /* This gets called on the inverse, virgin bitmap; optimize for sparse
        data. */

        if (v == 0xffffffff) {
            ret += 32;
            continue;
        }

        v -= ((v >> 1) & 0x55555555);
        v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
        ret += (((v + (v >> 4)) & 0xF0F0F0F) * 0x01010101) >> 24;
    }
    return ret;
#endif
}


#define FF(_b)  (0xffu << ((_b) << 3))

/* Count the number of bytes set in the bitmap. Called fairly sporadically,
   mostly to update the status screen or calibrate and examine confirmed
   new paths. */

u32 CountBytes( const u8* mem, u32 len) {
    return len - std::count(
      mem, std::next( mem, len ), u8( 0 )
    );
}


/* Count the number of non-255 bytes set in the bitmap. Used strictly for the
   status screen, several calls per second or so. */
u32 CountNon255Bytes( const u8* mem, u32 len) {
    return len - std::count(
      mem, std::next( mem, len ), u8( 255 )
    );
}

void MinimizeBits(u8* dst, const u8* src, u32 len) {
    u32 i = 0;
    while (i < len) {
        if (*(src++)) dst[i >> 3] |= 1 << (i & 7);
        i++;
    }
}

/* Helper function to compare buffers; returns first and last differing offset. We
   use this to find reasonable locations for splicing two files. */

std::tuple< s32, s32 > LocateDiffs( const u8* ptr1, const u8* ptr2, u32 len ) {
    s32 f_loc = -1;
    s32 l_loc = -1;
    for (u32 pos = 0; pos < len; pos++) {
        if (*(ptr1++) != *(ptr2++)) {
            if (f_loc == -1) f_loc = pos;
            l_loc = pos;
        }
    }
    return std::make_tuple( f_loc, l_loc );
}

constexpr const char *stacktrace_dumpfile = "./backtrace.dump";
constexpr int stacktrace_buffer_size = 65536;
#ifdef ENABLE_DANGEROUS_STACK_TRACE_OUTPUT
static char stacktrace_buffer[ stacktrace_buffer_size ] = { 0 };
#endif


static void backtrace_handler( int signum ) {
  ::signal( signum, SIG_DFL );
  boost::stacktrace::safe_dump_to( stacktrace_dumpfile );
#ifdef ENABLE_DANGEROUS_STACK_TRACE_OUTPUT
  constexpr const char *stacktrace_human_readable = "./backtrace.log";
  auto fd = open( stacktrace_human_readable, O_WRONLY|O_CREAT, 0600 );
  if( fd ) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    auto size = backtrace( reinterpret_cast< void** >( stacktrace_buffer ), stacktrace_buffer_size );
    backtrace_symbols_fd( reinterpret_cast< void** >( stacktrace_buffer ), size, fd );
#pragma GCC diagnostic pop
    close( fd );
  }
#endif
  ::raise( SIGABRT );
}

const set_segv_handler &set_segv_handler::get() {
  static const set_segv_handler instance;
  return instance;
}

set_segv_handler::set_segv_handler() {
#ifdef ENABLE_DISPLAY_LAST_STACK_TRACE
  if ( filesystem::exists( stacktrace_dumpfile ) ) {
    std::cout << "以前実行した際のバックトレース( " << stacktrace_dumpfile << " )が残っています" << std::endl;
    std::ifstream input( stacktrace_dumpfile );
    boost::stacktrace::stacktrace trace = boost::stacktrace::stacktrace::from_dump( input );
    std::cout << "Backtrace:" << std::endl;
    std::cout << trace << std::endl;
  }
#endif
  ::signal( SIGABRT, &backtrace_handler );
  ::signal( SIGSEGV, &backtrace_handler );
}


// TODO: multithread support
u64 GlobalCounter() {
    static u64 counter = 0;
    return counter++;
}

#ifdef FLC_FOUND
// FIXME: Common.cpp にあるべきクラスではない。GlobaLogger.cppなどに移したい。
namespace {
  class global_logger_t { // FIXME: 命名規則が既存のクラスと異なる
  public:
    static global_logger_t &get() {
      static global_logger_t instance;
      return instance;
    }
    ~global_logger_t() {
      if( fluent ) {
        guard.reset();
        thread->join();
        fluent.reset();
      }
    }
    void init(
      const std::string &url
    ) {
      fluent.reset( new flc::fluent_t( io_context, url ) );
      auto ppid = getppid();
      auto pid = getpid();
      std::string prefix( "wyvern." );
      prefix += boost::asio::ip::host_name();
      prefix += '.';
      boost::spirit::karma::generate( std::back_inserter( prefix ), boost::spirit::karma::uint_, ppid );
      prefix += '.';
      boost::spirit::karma::generate( std::back_inserter( prefix ), boost::spirit::karma::uint_, pid );
      fluent->set_tag_prefix( prefix );
      thread.reset( new std::thread([this](){ io_context.run(); }) );
    }
    template< typename T >
    fuzzuf::status_t write(
      std::string &&tag,
      T &&data
    ) {
      std::shared_ptr< std::promise< fuzzuf::status_t > > p( new std::promise< fuzzuf::status_t >() );
      auto f = p->get_future();
      (*fluent)[ std::move( tag ) ] << data << flc::commit(
        [p=std::move(p)]( auto result ) mutable {
          if( result == flc::status_t::OK );
          else if( result == flc::status_t::DISCONNECTED )
            std::cerr << "global_logger_t::write : fluentdに接続できない" << std::endl;
          else if( result == flc::status_t::CONFLICT )
            std::cerr << "global_logger_t::write : 他の処理と衝突した" << std::endl;
          else if( result == flc::status_t::BAD_REQUEST )
            std::cerr << "global_logger_t::write : 不正なリクエスト" << std::endl;
          else std::cerr << "global_logger_t::write : 不明" << std::endl;
          p->set_value( fuzzuf::status_cast< fuzzuf::status_t >( result ) );
        }
      );
      return f.get();
    }
    template< typename T >
    void write(
      std::string &&tag,
      T &&data,
      std::function< void( fuzzuf::status_t ) > &&cb
    ) {
      (*fluent)[ std::move( tag ) ] << data << flc::commit(
        [cb=std::move( cb )]( auto result ) {
          if( result == flc::status_t::OK );
          else if( result == flc::status_t::DISCONNECTED )
            std::cerr << "global_logger_t::write : fluentdに接続できない" << std::endl;
          else if( result == flc::status_t::CONFLICT )
            std::cerr << "global_logger_t::write : 他の処理と衝突した" << std::endl;
          else if( result == flc::status_t::BAD_REQUEST )
            std::cerr << "global_logger_t::write : 不正なリクエスト" << std::endl;
          else std::cerr << "global_logger_t::write : 不明" << std::endl;
          cb( fuzzuf::status_cast< fuzzuf::status_t >( result ) );
        }
      );
    }
    bool is_ready() const {
      return bool( fluent );
    }
  private:
    global_logger_t()
#if BOOST_VERSION < 106600
      : guard( new boost::asio::io_service::work( io_context ) ) {}
    boost::asio::io_service io_context;
    std::unique_ptr< boost::asio::io_service::work > guard;
#else
      : guard( boost::asio::make_work_guard( io_context ) ) {}
    boost::asio::io_context io_context;
    boost::asio::executor_work_guard< boost::asio::io_context::executor_type > guard;
#endif
    std::unique_ptr< flc::fluent_t > fluent;
    std::unique_ptr< std::thread > thread;
  };
}

void init_logger( const std::string &url ) {
  global_logger_t::get().init( url );
}

bool has_logger() {
  return global_logger_t::get().is_ready();
}

void log( std::string &&tag, std::string &&message, std::function< void( fuzzuf::status_t ) > &&cb ) {
  if( has_logger() )
    global_logger_t::get().write( std::move( tag ), std::move( message ), std::move( cb ) );
  else cb( fuzzuf::status_t::OK );
}

fuzzuf::status_t log( std::string &&tag, std::string &&message ) {
  if( has_logger() )
    return global_logger_t::get().write( std::move( tag ), std::move( message ) );
  else return fuzzuf::status_t::OK;
}

void log( std::string &&tag, nlohmann::json &&message, std::function< void( fuzzuf::status_t ) > &&cb ) {
  if( has_logger() )
    global_logger_t::get().write( std::move( tag ), std::move( message ), std::move( cb ) );
  else cb( fuzzuf::status_t::OK );
}

fuzzuf::status_t log( std::string &&tag, nlohmann::json &&message ) {
  if( has_logger() )
    return  global_logger_t::get().write( std::move( tag ), std::move( message ) );
  else return fuzzuf::status_t::OK;
}

#else

void init_logger( const std::string& ) {
}

bool has_logger() {
  return false;
}

void log( std::string&&, std::string&&, std::function< void( fuzzuf::status_t ) > &&cb ) {
  cb( fuzzuf::status_t::OK );
}

fuzzuf::status_t log( std::string&&, std::string&& ) {
  return fuzzuf::status_t::OK;
}

void log( std::string&&, nlohmann::json&&, std::function< void( fuzzuf::status_t ) > &&cb ) {
  cb( fuzzuf::status_t::OK );
}

fuzzuf::status_t log( std::string&&, nlohmann::json&& ) {
  return fuzzuf::status_t::OK;
}

#endif
};
