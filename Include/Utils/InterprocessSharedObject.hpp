#ifndef FUZZUF_INCLUDE_UTILS_INTERPROCESS_SHARED_OBJECT_HPP
#define FUZZUF_INCLUDE_UTILS_INTERPROCESS_SHARED_OBJECT_HPP
#include <new>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <sys/mman.h>
#include <boost/range/iterator_range.hpp>
#include <Utils/SharedRange.hpp>

namespace fuzzuf::utils::interprocess {

/*
 * T型の子プロセスと共有されるインスタンスを作る
 * TはPlain Old Dataの要件を満たさなければならない
 * インスタンスは引数で渡した値で初期化される
 *
 * 共有はMAP_SHARED|MAP_ANNONYMOUSなメモリをmmapする事で
 * 実現されている
 * 故にページサイズを下回るサイズ型であっても少なく
 * とも1ページは確保される事になる
 * また、この関数の呼び出しは確実にシステムコールを
 * 生じさせる
 * このため、小さい値を沢山共有する必要がある場合は
 * それらを構造体にまとめて大きな塊にしてから
 * create_shared_objectするのが望ましい
 *
 * 返り値はdeleterでmunmapするshared_ptr< T >
 *
 * mmapが失敗した場合(ex. 空きメモリがない)
 * 例外std::bad_allocが飛ぶ
 */
template< typename T >
auto create_shared_object( const T &v ) -> std::enable_if_t<
  std::is_pod_v< T >,
  std::shared_ptr< T >
> {
  auto addr = mmap(
    nullptr,
    sizeof( T ),
    PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_ANONYMOUS,
    -1,
    0
  );
  if( addr == reinterpret_cast< void* >( std::intptr_t( -1 ) ) )
    throw std::bad_alloc();
  return std::shared_ptr< T >(
    new( addr ) T( v ),
    [addr]( auto ) {
      munmap( addr, sizeof( T ) );
    }
  );
}

}
#endif

