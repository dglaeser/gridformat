// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <span>
#include <sstream>
#include <algorithm>

#include <gridformat/encoding/raw.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;

    "raw_binary_encoded_stream"_test = [] () {
        char bytes[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::ostringstream s;
        GridFormat::Encoding::raw(s).write(std::span{bytes});
        expect(std::ranges::equal(
            std::span{bytes},
            std::span{s.str().data(), s.str().size()}
        ));
    };

    return 0;
}