// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <algorithm>

#include <gridformat/common/accumulated_range.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;

    "accumulated_range"_test = [] () {
        std::vector<int> v{1, 2, 3, 4};
        const auto accumulated = GridFormat::AccumulatedRange{v};
        const auto expected = std::vector<std::size_t>{1, 3, 6, 10};
        expect(std::ranges::equal(accumulated, expected));
        expect(std::ranges::equal(accumulated, expected));
    };

    "accumulated_range_owning"_test = [] () {
        const auto accumulated = GridFormat::AccumulatedRange{std::vector<int>{1, 2, 3, 4}};
        const auto expected = std::vector<std::size_t>{1, 3, 6, 10};
        expect(std::ranges::equal(accumulated, expected));
        expect(std::ranges::equal(accumulated, expected));
    };

    return 0;
}