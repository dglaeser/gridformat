// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include <gridformat/common/extended_range.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::expect;
    using GridFormat::Testing::operator""_test;

    "extended_range"_test = [] () {
        std::vector<int> data{1, 2, 3};
        expect(std::ranges::equal(
            GridFormat::Ranges::extend_by(2, std::views::all(data)).with_value(0.0),
            std::vector<int>{1, 2, 3, 0, 0}
        ));
    };

    "extended_range_custom_extension_value"_test = [] () {
        std::vector<int> data{1, 2, 3};
        expect(std::ranges::equal(
            GridFormat::Ranges::extend_by(2, data).with_value(42),
            std::vector<int>{1, 2, 3, 42, 42}
        ));
    };

    "extended_range_owning_custom_extension_value"_test = [] () {
        expect(std::ranges::equal(
            GridFormat::Ranges::extend_by(2, std::vector<int>{1, 2, 3}).with_value(42),
            std::vector<int>{1, 2, 3, 42, 42}
        ));
    };

    return 0;
}
