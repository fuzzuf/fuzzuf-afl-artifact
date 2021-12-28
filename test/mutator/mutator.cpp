#define BOOST_TEST_MODULE mutator.mutator
#define BOOST_TEST_DYN_LINK
#include <string>
#include <boost/test/unit_test.hpp>

#include "ExecInput/OnMemoryExecInput.hpp"
#include "ExecInput/ExecInputSet.hpp"
#include "Mutator/Mutator.hpp"
#include "Utils/HexDump.hpp"

BOOST_AUTO_TEST_CASE(MutatorMutator) {
    // NOTE: 数値に意味はない。適当に生成した乱数。
    u8 buf_seed[] = {0x7d, 0xae,  0x9, 0x6, 0x40, 0xe9, 0x92, 0x5f};
    u8 buf_fuzz[] = {0x7d, 0xae, 0x1a, 0x6, 0x40, 0xe9, 0x92, 0x5f};
    //                           ~~~~
    //                            + 0x11

    ExecInputSet input_set; // to create OnMemoryInputSet, we need the set(factory)
    auto input = input_set.CreateOnMemory(buf_seed, sizeof(buf_seed));

    // 与えたバッファと内容が一致するのかを確認
    BOOST_CHECK(std::memcmp(input->GetBuf(), buf_seed, sizeof(buf_seed)) == 0);
    BOOST_CHECK_EQUAL(input->GetLen(), sizeof(buf_seed));

    auto mutator = Mutator(*input);
    int pos = 2;

    // 与えたバッファと内容が一致するのかを確認
    BOOST_CHECK(std::memcmp(mutator.GetBuf(), buf_seed, sizeof(buf_seed)) == 0);
    BOOST_CHECK_EQUAL(mutator.GetLen(), sizeof(buf_seed));

    // AddN<u8>(pos=2, val=1, be=false) して、所定のファズが生成されることを確認
    mutator.template AddN<u8>(pos, 0x11, (int) false);
    bool result_equal_to_given_fuzz = std::memcmp(mutator.GetBuf(), buf_fuzz, sizeof(buf_fuzz)) == 0;
    if (!result_equal_to_given_fuzz) {
        std::cout << "[!] Expected fuzz:  ";
        HexDump(stdout, buf_fuzz, sizeof(buf_fuzz), 0);
    }
    { // 常に表示
        std::cout << "[!] Generated fuzz: ";
        HexDump(stdout, mutator.GetBuf(), mutator.GetLen(), 0);
    }
    BOOST_CHECK(result_equal_to_given_fuzz);
    BOOST_CHECK_EQUAL(mutator.GetLen(), sizeof(buf_fuzz));
}
