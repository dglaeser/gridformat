// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <ranges>
#include <utility>
#include <algorithm>
#include <forward_list>

#include <gridformat/common/ranges.hpp>

#include "../testing.hpp"

template<std::ranges::sized_range R>
std::size_t get_sized_range_size(R&& r) {
    static_assert(!GridFormat::Concepts::StaticallySizedRange<R>);
    return GridFormat::Ranges::size(std::forward<R>(r));
}

template<std::ranges::range R>
std::size_t get_non_sized_range_size(R&& r) {
    static_assert(!std::ranges::sized_range<R>);
    static_assert(!GridFormat::Concepts::StaticallySizedRange<R>);
    return GridFormat::Ranges::size(std::forward<R>(r));
}

template<GridFormat::Concepts::StaticallySizedRange R>
constexpr std::size_t get_static_size_range(R&& r) {
    return GridFormat::Ranges::size(std::forward<R>(r));
}

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

// For 4d ranges, we get a maybe-uninitialized error on release builds
// & gcc-12 for which we couldn't yet figure out the reason.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
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
#pragma GCC diagnostic pop

    "sized_range_size"_test = [] () {
        expect(eq(get_sized_range_size(std::vector<int>(5)), 5_ul));
    };

    "non_sized_range_size"_test = [] () {
        std::forward_list<int> list{0, 1};
        expect(eq(get_non_sized_range_size(list), 2_ul));
    };

    "statically_sized_range_size"_test = [] () {
        static_assert(get_static_size_range(std::array<int, 2>{}) == 2);
    };

    return 0;
}
