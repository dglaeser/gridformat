// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <algorithm>

#include <gridformat/common/ranges.hpp>

#include "../testing.hpp"

int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "flat_1d_range_view_const"_test = [] () {
        const std::vector<int> values{0, 1, 2, 3, 4, 5};
        auto view = values | GridFormat::Views::flat;
        expect(std::ranges::equal(view, std::vector{0, 1, 2, 3, 4, 5}));
    };

    "flat_1d_range_view_non_const"_test = [] () {
        std::vector<int> values{0, 1, 2, 3, 4, 5};
        auto view = values | GridFormat::Views::flat;
        std::ranges::for_each(view, [&] (auto& v) { v = 0; });
        expect(std::ranges::equal(view, std::vector{0, 0, 0, 0, 0, 0}));
    };

    "flat_2d_range_view_const"_test = [] () {
        const std::vector<std::vector<int>> values{{0, 1}, {2, 3}, {4, 5}};
        auto view = values | GridFormat::Views::flat;
        expect(std::ranges::equal(view, std::vector{0, 1, 2, 3, 4, 5}));
    };

    "flat_2d_range_view_non_const"_test = [] () {
        std::vector<std::vector<int>> values{{0, 1}, {2, 3}, {4, 5}};
        auto view = values | GridFormat::Views::flat;
        std::ranges::for_each(view, [&] (auto& v) { v = 0; });
        expect(std::ranges::equal(view, std::vector{0, 0, 0, 0, 0, 0}));
    };

    "flat_3d_range_view_const"_test = [] () {
        const std::vector<std::vector<std::vector<int>>> values{
            {{0, 1}, {2, 3}}, {{4, 5}}
        };
        GridFormat::Ranges::FlatView view{values};
        expect(std::ranges::equal(view, std::vector{0, 1, 2, 3, 4, 5}));
    };

    "flat_3d_range_view_non_const"_test = [] () {
        std::vector<std::vector<std::vector<int>>> values{
            {{0, 1}, {2, 3}}, {{4, 5}}
        };
        GridFormat::Ranges::FlatView view{values};
        std::ranges::for_each(view, [&] (auto& v) { v = 0; });
        expect(std::ranges::equal(view, std::vector{0, 0, 0, 0, 0, 0}));
    };

    "flat_4d_range_view_const"_test = [] () {
        const std::vector<std::vector<std::vector<std::vector<int>>>> values{
            {{{0, 1}, {2, 3}}, {{4, 5}}},
            {{{6, 7}, {8, 9}}, {{10, 11}}}
        };
        GridFormat::Ranges::FlatView view{values};
        expect(std::ranges::equal(view, std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
    };

    "flat_4d_range_view_non_const"_test = [] () {
        std::vector<std::vector<std::vector<std::vector<int>>>> values{
            {{{0, 1}, {2, 3}}, {{4, 5}}},
            {{{6, 7}, {8, 9}}, {{10, 11}}}
        };
        GridFormat::Ranges::FlatView view{values};
        std::ranges::for_each(view, [&] (auto& v) { v = 0; });
        expect(std::ranges::equal(view, std::vector{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
    };

    return 0;
}
