#pragma once
#include <string>

// Check if non-decimal charactors does not exists to perform strict string to int conversion
bool CheckIfStringIsDecimal(std::string &str) {
    if (str.find_first_not_of("0123456789") != std::string::npos) {
        return false;
    }
    return true;
}

bool CheckIfStringIsDecimal(const char *cstr) {
    std::string str(cstr);
    return CheckIfStringIsDecimal(str);
}