#pragma once

#include <fmt/format.h>
#include <gi/string.hpp>

// This is all we need for gi::cstring to be printable
template <>
class fmt::formatter<gi::cstring, char>
    : public fmt::formatter<basic_string_view<char>, char>
{};
