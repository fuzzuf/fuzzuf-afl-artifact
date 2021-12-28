#ifndef FUZZUF_INCLUDE_UTILS_STATUS_HPP
#define FUZZUF_INCLUDE_UTILS_STATUS_HPP
#include <iostream>
#include <Utils/EnumCast.hpp>
namespace fuzzuf {

// 処理の結果等を成功失敗より詳しく返したい時に使う
// このプロダクト全体で”ステータス”と呼称されそうなものはすべてここにまとめるべき
enum class status_t {
  UNKNOWN, // 不明
  OK, // 成功
  BAD_REQUEST, // 要求の内容がおかしかったので処理は行われなかった
  CONFLICT, // 同時に行われた他の要求に先を越された為処理は中止された
  DISCONNECTED // 通信できない為処理は実行できなかった
};

// status_castがそれぞれの値の存在をチェックできるようにする
// これらのチェックを行うメタ関数は名前空間 detail::enum_cast::status に置かれる
FUZZUF_ENUM_CAST_CHECK( status, UNKNOWN )
FUZZUF_ENUM_CAST_CHECK( status, OK )
FUZZUF_ENUM_CAST_CHECK( status, BAD_REQUEST )
FUZZUF_ENUM_CAST_CHECK( status, CONFLICT )
FUZZUF_ENUM_CAST_CHECK( status, DISCONNECTED )

/*
 * status_cast< typename T, bool strict = true, typename U >( U value )を定義する
 * 使い方1:
 *   status_cast< 何らかのenum型 >( 何らかのenum型の値 )
 *   enum型であるU型の値valueを別のenum型であるT型の値に変換する
 *   T型は要素UNKNOWNを持つenum型でなければならない
 *   U型はenum型でなければならない
 *   返り値はT型の値になる
 *   変換は値ではなくenumの名前に基づいて行われる
 *   (U::HOGEはT::HOGEに変換される)
 *   Tに対応する型が無い場合strictがtrueかfalseかで挙動が変化する
 *   trueの場合(デフォルト): static_assertにかかりコンパイルが失敗する
 *   falseの場合: T::UNKNOWNが返る
 * 使い方2:
 *   status_cast< std::string >( 何らかのenum型の値 )
 *   enum型であるU型の値valueをstd::stringに変換する
 *   U型はenum型でなければならない
 *   返り値はenumの要素の名前を文字列にしたものになる
 *   (U::HOGEは"HOGE"に変換される)
 *   strictは無視される
 * 使い方3:
 *   status_cast< 何らかのenum型 >( 文字列 )
 *   std::stringの値valueをenum型であるT型の値に変換する
 *   T型は要素UNKNOWNを持つenum型でなければならない
 *   U型はstd::stringに変換可能な型でなければならない
 *   返り値はT型の値になる
 *   ("HOGE"はT::HOGEに変換される)
 *   T型の要素に文字列に対応する物が無い場合T::UNKNOWNが返る
 *   文字列の内容は実行時にならないと確定しないためstrictは無視される
 */
FUZZUF_ENUM_CAST_BEGIN( status, UNKNOWN ) {
  FUZZUF_ENUM_CAST_CONVERT( UNKNOWN )
  FUZZUF_ENUM_CAST_CONVERT( OK )
  FUZZUF_ENUM_CAST_CONVERT( BAD_REQUEST )
  FUZZUF_ENUM_CAST_CONVERT( CONFLICT )
  FUZZUF_ENUM_CAST_CONVERT( DISCONNECTED )
} FUZZUF_ENUM_CAST_END

// status_t型はostreamに流す事ができる
// デバッグ出力とテスト失敗時の出力用
template< typename Traits >
std::basic_ostream< char, Traits > &operator<<(
  std::basic_ostream<char, Traits> &l,
  status_t r
) {
  l << status_cast< std::string >( r );
  return l;
}

}

#endif

