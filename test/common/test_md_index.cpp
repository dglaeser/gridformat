// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <vector>
#include <sstream>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/md_index.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;
    using GridFormat::MDIndex;
    using GridFormat::MDIndexRange;

    "md_index_construct_from_range"_test = [] () {
        MDIndex index{std::views::iota(0, 3)};
        expect(eq(index.size(), std::size_t{3}));
        for (std::size_t i = 0; i < 3; ++i)
            expect(eq(index.get(i), i));
    };

    "md_index_construct_from_md_layout"_test = [] () {
        MDIndex index{GridFormat::MDLayout{std::vector{1, 1, 1}}};
        expect(eq(index.size(), std::size_t{3}));
    };

    "md_index_construct_from_integral"_test = [] () {
        MDIndex index{std::views::iota(0, 3)};
        expect(eq(index.size(), std::size_t{3}));
    };

    "md_index_construct_from_indices"_test = [] () {
        MDIndex index{std::vector<std::size_t>{1, 2, 3, 4}};
        expect(eq(index.size(), std::size_t{4}));
        for (std::size_t i = 0; i < 4; ++i)
            expect(eq(index.get(i), i + 1));
    };

    "md_index_construct_from_initializer_list"_test = [] () {
        MDIndex index{{2, 3, 4, 5}};
        expect(eq(index.size(), std::size_t{4}));
        for (std::size_t i = 0; i < 4; ++i)
            expect(eq(index.get(i), i + 2));
    };

    "md_index_setter"_test = [] () {
        MDIndex index{std::vector<std::size_t>{1, 2, 3, 4}};
        index.set(0, 42);
        expect(eq(index.get(0), std::size_t{42}));
        for (std::size_t i = 1; i < 4; ++i)
            expect(eq(index.get(i), i + 1));
    };

    "md_index_flat"_test = [] () {
        expect(eq(
            GridFormat::flat_index(MDIndex{{0}}, GridFormat::MDLayout{std::vector{1}}),
            std::size_t{0}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{1}}, GridFormat::MDLayout{std::vector{2}}),
            std::size_t{1}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{0, 1}}, GridFormat::MDLayout{std::vector{2, 2}}),
            std::size_t{1}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{1, 0}}, GridFormat::MDLayout{std::vector{2, 2}}),
            std::size_t{2}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{1, 1}}, GridFormat::MDLayout{std::vector{2, 2}}),
            std::size_t{3}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{1, 2, 3}}, GridFormat::MDLayout{std::vector{2, 3, 4}}),
            std::size_t{23}
        ));
    };

    "md_index_copy_constructor"_test = [] () {
        MDIndex index{{1, 2}};
        MDIndex other{index};
        index.set(0, 42);
        index.set(1, 42);
        expect(eq(index.get(0), std::size_t{42}));
        expect(eq(index.get(1), std::size_t{42}));
        expect(eq(other.get(0), std::size_t{1}));
        expect(eq(other.get(1), std::size_t{2}));
    };

    "md_index_copy_assignment"_test = [] () {
        MDIndex index{{1, 2}};
        MDIndex other = index;
        index.set(0, 42);
        index.set(1, 42);
        expect(eq(index.get(0), std::size_t{42}));
        expect(eq(index.get(1), std::size_t{42}));
        expect(eq(other.get(0), std::size_t{1}));
        expect(eq(other.get(1), std::size_t{2}));
    };

    "md_index_output"_test = [] () {
        MDIndex index{{1, 2}};
        std::ostringstream s;
        s << index;
        expect(eq(s.str(), std::string{"(1,2)"}));
    };

    "md_index_addition"_test = [] () {
        const MDIndex sum = MDIndex{{1, 2}} + MDIndex{{42, 43}};
        expect(sum == MDIndex{{43, 45}});
    };

    "md_index_inplace_addition"_test = [] () {
        MDIndex sum{{1, 2}};
        sum += MDIndex{{42, 43}};
        expect(sum == MDIndex{{43, 45}});
    };

    "md_index_range_1d"_test = [] () {
        static_assert(std::ranges::forward_range<MDIndexRange>);
        expect(std::ranges::equal(
            MDIndexRange{{4}},
            std::vector{
                MDIndex{0},
                MDIndex{1},
                MDIndex{2},
                MDIndex{3}
            }
        ));
    };

    "md_index_range_2d"_test = [] () {
        static_assert(std::ranges::forward_range<MDIndexRange>);
        expect(std::ranges::equal(
            MDIndexRange{{2, 3}},
            std::vector{
                MDIndex{0, 0}, MDIndex{1, 0},
                MDIndex{0, 1}, MDIndex{1, 1},
                MDIndex{0, 2}, MDIndex{1, 2}
            }
        ));
    };

    "md_index_range_3d"_test = [] () {
        static_assert(std::ranges::forward_range<MDIndexRange>);
        expect(std::ranges::equal(
            MDIndexRange{{2, 2, 1}},
            std::vector{
                MDIndex{0, 0, 0},
                MDIndex{1, 0, 0},
                MDIndex{0, 1, 0},
                MDIndex{1, 1, 0},
            }
        ));
    };

    return 0;
}
