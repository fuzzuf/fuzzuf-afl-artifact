#pragma once

#include <array>
#include "Utils/Common.hpp"

// To use a constexpr function as the initializer of count_class_lookup*,
// we need to put these 2 arrays into one struct
struct CountClasses {
    std::array<u8, 256>    lookup8;
    std::array<u16, 65536> lookup16;
};
