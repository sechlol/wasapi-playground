#include "common.h"
#include <string>

std::wstring string_to_wstring(const std::string& s)
{
    std::wstring temp;
    for (const auto& c : s) {
        temp += c;
    }
    return temp;
}