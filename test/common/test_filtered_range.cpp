// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <algorithm>

#include <gridformat/common/filtered_range.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;

    "filtered_range_first_true"_test = [] () {
        std::vector<int> v{1, 2, 3, 4, 0, 1, 8, 1};
        const auto filtered = GridFormat::Ranges::filter_by([] (int value) { return value < 3; }, v);
        const auto expected = std::vector<std::size_t>{1, 2, 0, 1, 1};
        expect(std::ranges::equal(filtered, expected));
        expect(std::ranges::equal(filtered, expected));
    };

    "filtered_range_none_true"_test = [] () {
        std::vector<int> v{1, 2, 3, 4, 1, 8, 1};
        const auto filtered = GridFormat::Ranges::filter_by([] (int value) { return value == 0; }, v);
        const auto expected = std::vector<std::size_t>{};
        expect(std::ranges::equal(filtered, expected));
        expect(std::ranges::equal(filtered, expected));
    };

    "filtered_range_owning"_test = [] () {
        const auto filtered = GridFormat::Ranges::filter_by(
            [] (int value) { return value < 3; },
            std::vector<int>{10, 1, 2, 3, 4, 0, 1, 8, 1}
        );
        const auto expected = std::vector<std::size_t>{1, 2, 0, 1, 1};
        expect(std::ranges::equal(filtered, expected));
        expect(std::ranges::equal(filtered, expected));
    };

    return 0;
}
