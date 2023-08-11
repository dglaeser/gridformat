// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iterator>
#include <algorithm>
#include <sstream>
#include <span>

#include <gridformat/encoding/base64.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    char in_data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::string expected{"AQIDBAUGBwgJ"};

    "base64_encoded_stream"_test = [&] () {
        std::ostringstream s;
        GridFormat::Encoding::base64(s).write(std::span{in_data});
        expect(eq(s.str(), expected));
    };

    "base64_encoded_stream_without_cache"_test = [&] () {
        std::ostringstream s;
        GridFormat::Encoding::base64({.num_cached_buffers=1})(s).write(std::span{in_data});
        expect(eq(s.str(), expected));
    };

    "base64_encoded_stream_with_small_cache"_test = [&] () {
        std::ostringstream s;
        GridFormat::Encoding::base64.with({.num_cached_buffers=20})(s).write(std::span{in_data});
        expect(eq(s.str(), expected));
    };

    "base64_decode"_test = [&] () {
        std::ostringstream s;
        GridFormat::Encoding::base64(s).write(std::span{in_data});

        std::vector<char> decoded;
        std::ranges::copy(s.str(), std::back_inserter(decoded));

        const auto size = GridFormat::Base64Decoder{}.decode(std::span{decoded});
        decoded.resize(size);
        expect(std::ranges::equal(decoded, in_data));
    };

    return 0;
}
