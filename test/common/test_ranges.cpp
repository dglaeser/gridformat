// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <ranges>
#include <utility>
#include <algorithm>
#include <type_traits>
#include <forward_list>

#include <gridformat/common/ranges.hpp>

#include "../testing.hpp"

template<int dim>
struct MDVectorFactory;

template<> struct MDVectorFactory<1> {
    static auto make(int size) { return std::vector<int>(size, 0); }
};

template<> struct MDVectorFactory<2> {
    static auto make(int size_x, int size_y) {
        return std::vector<std::vector<int>>(size_x, MDVectorFactory<1>::make(size_y));
    }
};

template<int... extents>
void test_flat_range_view() {
    int i = 0;
    auto storage = MDVectorFactory<sizeof...(extents)>::make(extents...);
    std::ranges::for_each(storage | GridFormat::Views::flat, [&] (auto& value) {
        value = i++;
    });

    const auto const_storage = storage;
    GridFormat::Testing::expect(std::ranges::equal(
        const_storage | GridFormat::Views::flat,
        std::views::iota(0, i)
    ));
}

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

    "flat_1d_range_view"_test = [] () {
        test_flat_range_view<5>();
    };

    "flat_2d_range_view"_test = [] () {
        test_flat_range_view<3, 4>();
    };

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
