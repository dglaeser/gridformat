// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <sstream>
#include <string>
#include <vector>
#include <span>

#include <gridformat/encoding/ascii.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "ascii_encoded_stream"_test = [] () {
        std::ostringstream s;
        std::vector<int> v{1, 2, 3, 42};
        auto ascii_stream = GridFormat::Encoding::ascii(s);
        ascii_stream.write(std::span{v});
        expect(eq(s.str(), std::string{"12342"}));
    };

    "ascii_encoded_formatted_stream"_test = [] () {
        std::ostringstream s;
        std::vector<int> v{1, 2, 3, 42};
        auto ascii_stream = GridFormat::Encoding::Ascii::with({.delimiter = ","})(s);
        ascii_stream.write(std::span{v});
        expect(eq(s.str(), std::string{"1,2,3,42,"}));
    };

    return 0;
}
