#include "Mutator/Mutator.hpp"

#include <cassert>
#include <random>

#include "Algorithms/AFL/AFLDictData.hpp"
#include "Algorithms/AFL/AFLUtil.hpp"
#include "Utils/Common.hpp"
#include "ExecInput/ExecInput.hpp"
#include "Logger/Logger.hpp"

#define INTERESTING_8 \
  -128,          /* Overflow signed 8-bit when decremented  */ \
  -1,            /*                                         */ \
   0,            /*                                         */ \
   1,            /*                                         */ \
   16,           /* One-off with common buffer size         */ \
   32,           /* One-off with common buffer size         */ \
   64,           /* One-off with common buffer size         */ \
   100,          /* One-off with common buffer size         */ \
   127           /* Overflow signed 8-bit when incremented  */

#define INTERESTING_16 \
  -32768,        /* Overflow signed 16-bit when decremented */ \
  -129,          /* Overflow signed 8-bit                   */ \
   128,          /* Overflow signed 8-bit                   */ \
   255,          /* Overflow unsig 8-bit when incremented   */ \
   256,          /* Overflow unsig 8-bit                    */ \
   512,          /* One-off with common buffer size         */ \
   1000,         /* One-off with common buffer size         */ \
   1024,         /* One-off with common buffer size         */ \
   4096,         /* One-off with common buffer size         */ \
   32767         /* Overflow signed 16-bit when incremented */

#define INTERESTING_32 \
  -2147483648LL, /* Overflow signed 32-bit when decremented */ \
  -100663046,    /* Large negative number (endian-agnostic) */ \
  -32769,        /* Overflow signed 16-bit                  */ \
   32768,        /* Overflow signed 16-bit                  */ \
   65535,        /* Overflow unsig 16-bit when incremented  */ \
   65536,        /* Overflow unsig 16 bit                   */ \
   100663045,    /* Large positive number (endian-agnostic) */ \
   2147483647    /* Overflow signed 32-bit when incremented */

const std::vector<s8>  Mutator::interesting_8  { INTERESTING_8 };
const std::vector<s16> Mutator::interesting_16 { INTERESTING_8,
                                                 INTERESTING_16 };
const std::vector<s32> Mutator::interesting_32 { INTERESTING_8,
                                                 INTERESTING_16,
                                                 INTERESTING_32 };

#undef INTERESTING_8
#undef INTERESTING_16
#undef INTERESTING_32

/* TODO: Implement generator class */
#define FLIP_BIT(_ar, _b) do { \
    u8* _arf = (u8*)(_ar); \
    u32 _bf = (_b); \
    _arf[(_bf) >> 3] ^= (128 >> ((_bf) & 7)); \
  } while (0)

Mutator::Mutator( const ExecInput &input ) :
        input( input ),
        len( input.GetLen() ),
        outbuf( new u8[len] ),
        tmpbuf( nullptr ),
        temp_len( 0 ),
        splbuf( nullptr ),
        spl_len( 0 ),
        rand_fd( Util::OpenFile("/dev/urandom", O_RDONLY | O_CLOEXEC) )
{
    std::memcpy(outbuf, input.GetBuf(), len);
}

Mutator::~Mutator() {
    if (outbuf) delete[] outbuf;
    if (tmpbuf) delete[] tmpbuf;
    if (splbuf) delete[] splbuf;
    if (rand_fd != -1) {
      Util::CloseFile(rand_fd);
      rand_fd = -1;
    }
}

Mutator::Mutator(Mutator&& src):
        input( src.input ),
        len( src.len ),
        outbuf( src.outbuf ),
        tmpbuf( src.tmpbuf ),
        temp_len( src.temp_len),
        splbuf( src.splbuf ),
        spl_len( src.spl_len ),
        rand_fd( src.rand_fd )
{
    src.outbuf = nullptr;
    src.tmpbuf = nullptr;
    src.splbuf = nullptr;
    src.rand_fd = -1;
}

const ExecInput& Mutator::GetSource() {
  return input;
}

/* Helper to choose random block len for block operations in fuzz_one().
   Doesn't return zero, provided that max_len is > 0. */

u32 Mutator::ChooseBlockLen(u32 limit) {
    u32 min_value, max_value;
    u32 rlim = 3ULL; 

    // just an alias of afl::util::UR
    auto UR = [this](u32 limit) {
        return afl::util::UR(limit, rand_fd);
    };

    switch (UR(rlim)) {
    case 0:  min_value = 1;
             max_value = AFLOption::HAVOC_BLK_SMALL;
             break;

    case 1:  min_value = AFLOption::HAVOC_BLK_SMALL;
             max_value = AFLOption::HAVOC_BLK_MEDIUM;
             break;
    default: 
        if (UR(10)) {
            min_value = AFLOption::HAVOC_BLK_MEDIUM;
            max_value = AFLOption::HAVOC_BLK_LARGE;
        } else {
            min_value = AFLOption::HAVOC_BLK_LARGE;
            max_value = AFLOption::HAVOC_BLK_XL;
        }
    }

    if (min_value >= limit) min_value = 1;

    return min_value + UR(std::min(max_value, limit) - min_value + 1);
}

// FIXME: 以下の全ミューテーションアルゴリズムにバッファオーバーランの疑いあり。要修正
// 内部ロジックに詳しいのがMutatorクラスのはずなので、値チェックの責務はMutatorにあると思います。
// パフォーマンスを気にしてこの実装になっているなら、debugビルドのときだけassertが有効になるdebug_assertを用意すればいい

u32 Mutator::OverwriteWithSet(u32 pos, const std::vector<char> &char_set) {
  std::vector<char> out;
  std::sample(
      char_set.begin(),
      char_set.end(),
      std::back_inserter(out),
      1,
      mt_engine
  );
  outbuf[pos] = out[0];
  return 1;
}

u32 Mutator::FlipBit(u32 pos, int n) {
    /* Flip bits */ 

    for (int i = 0; i < n; i++) { // pos + n - 1 > sizeof(outbuf) とのとき、バッファオーバーランが発生？
        u32 flip = pos + i;
        FLIP_BIT(outbuf, flip);
    }

    return 1;
}

u32 Mutator::FlipByte(u32 pos, int n) {
    /* Flip bytes */    

    // why we don't *(T*)(outbuf + pos) ^= T(1 << n) - 1; ?
    // it can be unaligned memory access
    if (n == 1) {
        outbuf[pos] ^= 0xff;
    } else if (n == 2) {
        u16 v = ReadMem<u16>(pos);
        Overwrite<u16>(pos, (v ^ 0xFFFF) );
    } else if (n == 4) {
        u32 v = ReadMem<u32>(pos);
        Overwrite<u32>(pos, (v ^ 0xFFFFFFFF) );
    }

    return 1;    
}

void Mutator::Replace(int pos, u8 *buf, u32 len) {
    std::memcpy(outbuf+pos, buf, len);
}

void Mutator::Havoc(
    u32 stacking, 
    const std::vector<AFLDictData> &extras, 
    const std::vector<AFLDictData> &a_extras
) {
    DEBUG("Havoc(%u) <Before>\n", stacking);

    // FIXME: all the operations can be rewritten with vector<u8>

    if (temp_len < len) {
        delete[] tmpbuf;
        tmpbuf = nullptr;
    }

    if (!tmpbuf) {
        tmpbuf = new u8[len];
    }

    std::memcpy(tmpbuf, outbuf, len);
    temp_len = len;
 
    // swap outbuf and tmpbuf to use mutation function on tmpbuf
    // also, if we want to restore the original buffer
    // we can just swap them again
    std::swap(outbuf, tmpbuf);
    std::swap(len, temp_len);

    // just an alias of afl::util::UR
    auto UR = [this](u32 limit) {
        return afl::util::UR(limit, rand_fd);
    };

    for (std::size_t i = 0; i < stacking; i++) {
        const u32 num_cases = (extras.empty() && a_extras.empty()) ? 15 : 17;
        switch (UR(num_cases)) {
        case 0:
            /* Flip a single bit somewhere. Spooky! */
            FlipBit(UR(len << 3), 1);
            break;
        case 1: 
            /* Set byte to interesting value. */
#ifdef BEHAVE_DETERMINISTIC
            {
                auto a = UR(len);
                auto b = UR(interesting_8.size());
                InterestN<u8>(a, b, false);
            }
#else
            InterestN<u8>(UR(len), UR(interesting_8.size()), false);
#endif
            break;
        case 2:
            /* Set word to interesting value, randomly choosing endian. */
            if (len < 2) break;
#ifdef BEHAVE_DETERMINISTIC
            {
                auto c = !UR(2);
                auto a = UR(len - 1);
                auto b = UR(interesting_16.size());
                InterestN<u16>(a, b, c);
            }
#else
            InterestN<u16>(UR(len - 1), UR(interesting_16.size()), !UR(2));
#endif
            break;
        case 3:
            /* Set dword to interesting value, randomly choosing endian. */
            if (len < 4) break;
#ifdef BEHAVE_DETERMINISTIC
            {
                auto c = !UR(2);
                auto a = UR(len - 3);
                auto b = UR(interesting_32.size());
                InterestN<u32>(a, b, c);
            }
#else
            InterestN<u32>(UR(len - 3), UR(interesting_32.size()), !UR(2));
#endif
            break;
        case 4:
            /* Randomly subtract from byte. */
#ifdef BEHAVE_DETERMINISTIC
            {
                auto a = UR(len);
                auto b = 1 + UR(AFLOption::ARITH_MAX);
                SubN<u8>(a, b, false);
            }
#else
            SubN<u8>(UR(len), 1 + UR(AFLOption::ARITH_MAX), false);
#endif
            break;
        case 5:
            /* Randomly add to byte. */
#ifdef BEHAVE_DETERMINISTIC
            {
                auto a = UR(len);
                auto b = 1 + UR(AFLOption::ARITH_MAX);
                AddN<u8>(a, b, false);
            }
#else
            AddN<u8>(UR(len), 1 + UR(AFLOption::ARITH_MAX), false);
#endif
            break;
        case 6:
            /* Randomly subtract from word, random endian. */
            if (len < 2) break;
#ifdef BEHAVE_DETERMINISTIC
            {
                auto c = !UR(2);
                auto a = UR(len - 1);
                auto b = 1 + UR(AFLOption::ARITH_MAX);
                SubN<u16>(a, b, c);
            }
#else
            SubN<u16>(UR(len - 1), 1 + UR(AFLOption::ARITH_MAX), !UR(2));
#endif
            break;
        case 7:
            /* Randomly add to word, random endian. */
            if (len < 2) break;
#ifdef BEHAVE_DETERMINISTIC
            {
                auto c = !UR(2);
                auto a = UR(len - 1);
                auto b = 1 + UR(AFLOption::ARITH_MAX);
                AddN<u16>(a, b, c);
            }
#else
            AddN<u16>(UR(len - 1), 1 + UR(AFLOption::ARITH_MAX), !UR(2));
#endif
            break;
        case 8:
            /* Randomly subtract from dword, random endian. */
            if (len < 4) break;
#ifdef BEHAVE_DETERMINISTIC
            {
                auto c = !UR(2);
                auto a = UR(len - 3);
                auto b = 1 + UR(AFLOption::ARITH_MAX);
                SubN<u32>(a, b, c);
            }
#else
            SubN<u32>(UR(len - 3), 1 + UR(AFLOption::ARITH_MAX), !UR(2));
#endif
            break;
        case 9:
            /* Randomly add to dword, random endian. */
            if (len < 4) break;
#ifdef BEHAVE_DETERMINISTIC
            {
                auto c = !UR(2);
                auto a = UR(len - 3);
                auto b = 1 + UR(AFLOption::ARITH_MAX);
                AddN<u32>(a, b, c);
            }
#else
            AddN<u32>(UR(len - 3), 1 + UR(AFLOption::ARITH_MAX), !UR(2));
#endif
            break;
        case 10:
          /* Just set a random byte to a random value. Because,
             why not. We use XOR with 1-255 to eliminate the
             possibility of a no-op. */
#ifdef BEHAVE_DETERMINISTIC
             {
                auto a = 1 + UR(255);
                outbuf[UR(len)] ^= a;
             }
#else
             outbuf[UR(len)] ^= 1 + UR(255);
#endif
             break;
        case 11 ... 12: {
            /* Delete bytes. We're making this a bit more likely
               than insertion (the next option) in hopes of keeping
               files reasonably small. */

            u32 del_from, del_len;
            if (len < 2) break;

            /* Don't delete too much. */
            del_len = ChooseBlockLen(len - 1);
            del_from = UR(len - del_len + 1);
            std::memmove(outbuf + del_from, outbuf + del_from + del_len,
                     len - del_from - del_len);
            len -= del_len;
            break;
        }
        case 13:
            if (len + AFLOption::HAVOC_BLK_XL < AFLOption::MAX_FILE) {
            /* Clone bytes (75%) or insert a block of constant bytes (25%). */
                u8  actually_clone = UR(4);
                u32 clone_from, clone_to, clone_len;
                u8* new_buf;
                if (actually_clone) {
                    clone_len  = ChooseBlockLen(len);
                    clone_from = UR(len - clone_len + 1);
                } else {
                    clone_len = ChooseBlockLen(AFLOption::HAVOC_BLK_XL);
                    clone_from = 0;
                }

                clone_to   = UR(len);

                new_buf = new u8[len + clone_len];

                /* Head */
                std::memcpy(new_buf, outbuf, clone_to);
                /* Inserted part */
                if (actually_clone)
                    std::memcpy(new_buf + clone_to, outbuf + clone_from, clone_len);
                else
                    std::memset(new_buf + clone_to,
                                UR(2) ? UR(256) : outbuf[UR(len)], clone_len);
                /* Tail */
                std::memcpy(new_buf + clone_to + clone_len, outbuf + clone_to,
                    len - clone_to);
                delete[] outbuf;
                outbuf = new_buf;
                len += clone_len;
            }
            break;
        case 14: {
            /* Overwrite bytes with a randomly selected chunk (75%) or fixed
               bytes (25%). */
            u32 copy_from, copy_to, copy_len;

            if (len < 2) break;

            copy_len  = ChooseBlockLen(len - 1);
            copy_from = UR(len - copy_len + 1);
            copy_to   = UR(len - copy_len + 1);

            if (UR(4)) {
                if (copy_from != copy_to)
                    std::memmove(outbuf + copy_to, outbuf + copy_from, copy_len);

            } else std::memset(outbuf + copy_to,
                          UR(2) ? UR(256) : outbuf[UR(len)], copy_len);
            break;
        }

        /* Values 15 and 16 can be selected only if there are any extras
           present in the dictionaries. */
        case 15: {
            /* Overwrite bytes with an extra. */

            bool use_auto = extras.empty() || (!a_extras.empty() && UR(2));
            u32 idx = use_auto ? UR(a_extras.size()) : UR(extras.size());
            const AFLDictData &extra = use_auto ? a_extras[idx] : extras[idx];

            u32 extra_len = extra.data.size();
            if (extra_len > len) break;

            u32 insert_at = UR(len - extra_len + 1);
            std::memcpy(outbuf + insert_at, &extra.data[0], extra_len);

            break;
        }

        case 16: {
            u32 insert_at = UR(len + 1);

            /* Insert an extra. Do the same dice-rolling stuff as for the
               previous case. */

            bool use_auto = extras.empty() || (!a_extras.empty() && UR(2));
            u32 idx = use_auto ? UR(a_extras.size()) : UR(extras.size());
            const AFLDictData &extra = use_auto ? a_extras[idx] : extras[idx];

            u32 extra_len = extra.data.size();

            if (len + extra_len >= AFLOption::MAX_FILE) break;

            u8* new_buf = new u8[len + extra_len];

            /* Head */
            std::memcpy(new_buf, outbuf, insert_at);

            /* Inserted part */
            std::memcpy(new_buf + insert_at, &extra.data[0], extra_len);

            /* Tail */
            std::memcpy(new_buf + insert_at + extra_len, outbuf + insert_at,
                   len - insert_at);

            delete[] outbuf;
            outbuf   = new_buf;
            len += extra_len;
            
            break;
        }
        }
    }
}

// FIXME: add a test which uses this with AFLMutationHierarFlowRoutines
void Mutator::RestoreHavoc(void) {
    std::swap(outbuf, tmpbuf);
    std::swap(len, temp_len);
}

// return true if the splice occurred, and false otherwise
bool Mutator::Splice(const ExecInput& target) {
    auto cmp_len = std::min(len, target.GetLen());
    auto [f_diff, l_diff] = Util::LocateDiffs(outbuf, target.GetBuf(), cmp_len);

    if (f_diff < 0 || l_diff < 2 || f_diff == l_diff) return false;

    /* Split somewhere between the first and last differing byte. */

    u32 split_at = f_diff + afl::util::UR(l_diff - f_diff, rand_fd);

    /* Do the thing. */

    if (!splbuf) {
        splbuf = new u8[target.GetLen()];
    } else if (spl_len < target.GetLen()) {
        // We reallocate a buffer only when its size is not enough
        delete[] splbuf;
        splbuf = new u8[target.GetLen()];
    }

    spl_len = target.GetLen();
    std::memcpy(splbuf, outbuf, split_at);
    std::memcpy(splbuf+split_at, target.GetBuf()+split_at, spl_len - split_at);

    std::swap(outbuf, splbuf);
    std::swap(len, spl_len);
    return true;
}

// FIXME: add a test which uses this with AFLMutationHierarFlowRoutines
void Mutator::RestoreSplice(void) {
    std::swap(outbuf, splbuf);
    std::swap(len, spl_len);
}

// special case: we don't need to care about alignment on u8
template<>
u8 Mutator::ReadMem<u8>(u32 pos) {
    return outbuf[pos];
}

// special case: we don't need to care about alignment on u8
template<>
u32 Mutator::Overwrite<u8>(u32 pos, u8 chr) {
    outbuf[pos] = chr;
    return 1;
}

#undef FLIP_BIT
