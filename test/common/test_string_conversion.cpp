// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <algorithm>

#include <gridformat/common/string_conversion.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "string_literal_to_string_conversion"_test = [] () {
        const auto hello_world = GridFormat::as_string("hello, world");
        expect(std::ranges::equal(hello_world + '\0', "hello, world"));
    };

    "string_to_string_conversion"_test = [] () {
        const auto hello_world = GridFormat::as_string(std::string{"hello, world"});
        expect(std::ranges::equal(hello_world, std::string{"hello, world"}));
    };

    "scalar_to_string_conversion"_test = [] () {
        const auto one = GridFormat::as_string(int{1});
        expect(eq(one, std::string{"1"}));
    };

    "range_to_string_conversion"_test = [] () {
        const auto values = GridFormat::as_string(std::vector<int>{1, 2, 3, 4});
        expect(eq(values, std::string{"1 2 3 4"}));
    };

    "range_to_string_conversion_custom_delimiter"_test = [] () {
        const auto values = GridFormat::as_string(std::vector<int>{1, 2, 3, 4}, ",");
        expect(eq(values, std::string{"1,2,3,4"}));
    };

    "2d_range_to_string_conversion_custom_delimiter"_test = [] () {
        const auto values = GridFormat::as_string(
            std::vector<std::vector<int>>{
                {1, 2, 3},
                {4, 5, 6}
            }
        );
        expect(eq(values, std::string{"1 2 3 4 5 6"}));
    };

    return 0;
}
